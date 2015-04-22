/* While using this DMA interface, you will provide your own function pointer to our get_channel function which will do all the 
initialization work. */ 

#include <sys/param.h> 
#include <sys/systm.h> 
#include <sys/bus.h> 
#include <sys/kernel.h> 
#include <sys/lock.h> 
#include <sys/malloc.h> 
#include <sys/module.h> 
#include <sys/mutex.h> 
#include <sys/resource.h> 
#include <sys/rman.h> 

#include <machine/bus.h> 

#include <dev/ofw/ofw_bus.h>
#include <dev/ofw/ofw_bus_subr.h>

#include "awin_dma.h" 

/* No of total channels for Dedicated and Nomal DMA */ 
#define NNDMA 8 
#define NDDMA 8 

enum a10_dma_channel_type { 
	NDMA, 
	DDMA
} ; 


struct a10_dma_softc { 
	device_t a10_dma_dev ; 
	struct mtx a10_dma_mtx ; 
	bus_space_tag_t a10_dma_bst ; 
	bus_space_handle_t a10_dma_bsh ; 
	struct resource* a10_dma_mem_resource ; 
	struct resource* a10_dma_irq_resource ; 
	int a10_dma_mem_rid ; 
	int a10_dma_irq_rid ; 
	void* a10_dma_intrhand ; 
	#define BUFF_SIZE 64 
} ; 

struct a10_dma_channel { 
	struct mtx a10_mtx ; 
	struct a10_dma_softc* sc ; 
	bus_dma_tag_t a10_dma_tag ; 
	bus_dmamap_t a10_dma_map ; 
	uint32_t buff ; 
	uint32_t a10_dma_busaddr ; 
	enum a10_dma_channel_type a10_dma_type ; 
	uint8_t in_use ; 
} ; 

struct a10_dma_controller { 
	struct a10_dma_softc* sc ; 
	struct mtx a10_dma_controller_mtx ; 
	struct a10_dma_channel ddma_channels[NDDMA] ; 
	struct a10_dma_channel ndma_channels[NNDMA] ; 
	uint32_t ndma_channels_in_use ; 
	uint32_t ddma_channels_in_use ; 
} ; 

static int a10_dma_probe(device_t) ; 
static int a10_dma_attach(device_t) ; 
static int a10_dma_detach(device_t) ; 
static void a10_dma_release_resources(device_t) ; 
static void a10_dma_intr(void*) ; 
struct a10_dma_channel* get_a10_dma_channel() ; /* Currently will only work for Dedicated DMA channel types. */ 

static int a10_dma_probe(device_t dev) 
{
	if(!ofw_bus_status_okay(dev))
		return (ENXIO) ; 
	if(!ofw_bus_is_compatible(dev, "allwinner,sun4i-a10-dma"))
		return (ENXIO) ; 

	device_set_desc(dev, "Allwinner DMA Controller") ;  	

	return (BUS_PROBE_DEFAULT) ; 
}

static int a10_dma_attach(device_t dev)
{
	struct a10_dma_softc* sc;  
	sc = device_get_softc(dev) ; 
	sc->a10_dma_dev = dev ; 

	mtx_init(&sc->a10_dma_mtx, device_get_nameunit(sc->a10_dma_dev),"a10_dma", MTX_DEF) ;  

	sc->a10_dma_mem_resource = bus_alloc_resource_any(sc->a10_dma_dev, SYS_RES_MEMORY, &sc->a10_dma_mem_rid, RF_ACTIVE) ; 

	if(sc->a10_dma_mem_resource == NULL) { 
		device_printf(dev, "Cannot allocate memory resource !\n") ; 
		a10_dma_release_resources(dev) ; 
		return (ENXIO) ; 
	}

	sc->a10_dma_bst = rman_get_bustag(sc->a10_dma_mem_resource) ; 
	sc->a10_dma_bsh = rman_get_bushandle(sc->a10_dma_mem_resource) ; 

	sc->a10_dma_irq_resource = bus_alloc_resource_any(sc->a10_dma_dev, SYS_RES_IRQ, &sc->a10_dma_irq_rid, RF_ACTIVE | RF_SHAREABLE) ; 
	
	if(sc->a10_dma_irq_resource == NULL) { 
		device_printf(dev, "Cannot allocate irq resource!\n") ; 
		a10_dma_release_resources(dev) ; 
		return (ENXIO) ; 
	}

	if(bus_setup_intr(sc->a10_dma_dev, sc->a10_dma_irq_resource, INTR_MPSAFE | INTR_TYPE_MISC, NULL,a10_dma_intr, sc, &sc->a10_dma_intrhand)) { 
		device_printf(dev, "Cannot setup interrupt handler!\n") ; 
		a10_dma_release_resources(dev) ; 
		return (ENXIO) ; 
	}


	sc->a10_dma_intrhand = a10_dma_intr ; 
	
	/* Question :- Do we have to setup a clock for DMA or not ? */ 
	/* Question :- What more do we have to write in here. */ 
	
	return (0) ; 
}  

/* It is the responsibility of the allocater of DMA channel to deallocate its resources by making a call to the functions provided by our interface. 
*/ 

static int a10_dma_detach(device_t dev)
{
	a10_dma_release_resources(dev) ; 
	return (0) ; 
} 

static void a10_dma_release_resources(device_t dev)
{
	struct a10_dma_softc* sc = device_get_softc(dev) ; 
	
	if(sc->a10_dma_mem_resource != NULL) 
		bus_release_resource(dev, SYS_RES_MEMORY,sc->a10_dma_mem_rid, sc->a10_dma_mem_resource) ; 

	if(sc->a10_dma_irq_resource != NULL) { 
		bus_teardown_intr(dev, sc->a10_dma_mem_resource, sc->a10_dma_intrhand) ; 
		bus_release_resource(dev, SYS_RES_IRQ, sc->a10_dma_irq_rid, sc->a10_dma_irq_resource) ; 
	} 
}

static void a10_dma_intr(void* ptr)
{
	return  ; 
}

static device_method_t a10_dma_methods[] = { 
	DEVMETHOD(device_probe, a10_dma_probe),
	DEVMETHOD(device_attach, a10_dma_attach),
	DEVMETHOD(device_detach, a10_dma_detach),

	DEVMETHOD_END
} ; 

static devclass_t a10_dma_devclass ; 

static driver_t a10_dma_driver = {  
	"a10_dma", 
	a10_dma_methods,
	sizeof(struct a10_dma_softc)
} ; 

DRIVER_MODULE(a10_dma, simplebus, a10_dma_driver ,a10_dma_devclass,0,0) ; 
