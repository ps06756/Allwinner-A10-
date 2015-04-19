/*-
 * Copyright (c) 2013 Alexander Fedorov
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD");

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

#include <dev/mmc/bridge.h>
#include <dev/mmc/mmcreg.h>
#include <dev/mmc/mmcbrvar.h>

#include <arm/allwinner/a10_clk.h>
#include <arm/allwinner/a10_mmc.h>
#include <arm/allwinner/awin_dma.h>

struct a10_mmc_softc {
	device_t		a10_dev;
	struct mtx		a10_mtx;
	bus_space_tag_t 	a10_bst ; 
	bus_space_handle_t a10_bsh  ; 
	struct resource *	a10_mem_res;
	struct resource *	a10_irq_res;
	struct a10_dma_softc 	dma_sc ; 
	uint8_t 		use_dma ; 
	void *			a10_intrhand;
	struct mmc_host		a10_host;
	struct mmc_request *	a10_req;
	uint32_t		mod_clk;
	int			a10_bus_busy;
};

static int a10_mmc_probe(device_t);
static int a10_mmc_attach(device_t);
static int a10_mmc_detach(device_t);
static int a10_mmc_dma_attach(device_t)  ; 
static void a10_mmc_callback(void* arg, bus_dma_segment_t* segs, int nseg, int error) ; 
static void a10_mmc_free_resources(struct a10_mmc_softc*) ; 
static void a10_mmc_intr(void *);

static int a10_mmc_update_ios(device_t, device_t);
static int a10_mmc_request(device_t, device_t, struct mmc_request *);
static int a10_mmc_get_ro(device_t, device_t);
static int a10_mmc_acquire_host(device_t, device_t);
static int a10_mmc_release_host(device_t, device_t);


#define	a10_mmc_lock(_sc)	mtx_lock(&(_sc)->a10_mtx)
#define	a10_mmc_unlock(_sc)	mtx_unlock(&(_sc)->a10_mtx)
#define	a10_mmc_read_4(_sc, _reg)					\
	bus_space_read_4((_sc)->a10_bst, (_sc)->a10_bsh, _reg)
#define	a10_mmc_write_4(_sc, _reg, _value)				\
	bus_space_write_4((_sc)->a10_bst, (_sc)->a10_bsh, _reg, _value)

static int
a10_mmc_probe(device_t dev)
{

	if (!ofw_bus_status_okay(dev))
		return (ENXIO);
	if (!ofw_bus_is_compatible(dev, "allwinner,sun4i-a10-mmc"))
		return (ENXIO);
	device_set_desc(dev, "Allwinner Integrated MMC/SD controller");

	return (BUS_PROBE_DEFAULT);
}

static int
a10_mmc_attach(device_t dev)
{
	device_t child;
	int rid;
	struct a10_mmc_softc *sc;
	uint32_t reg;
	uint32_t time_left = 0xFFFF ; 

	sc = device_get_softc(dev);
	sc->a10_dev = dev;
	sc->a10_req = NULL;

	mtx_init(&sc->a10_mtx, device_get_nameunit(sc->a10_dev), "a10_mmc", MTX_DEF);

	rid = 0;
	sc->a10_mem_res = bus_alloc_resource_any(dev, SYS_RES_MEMORY, &rid,
	    RF_ACTIVE);
	if (!sc->a10_mem_res) {
		device_printf(dev, "cannot allocate memory window\n");
		return (ENXIO);
	}

	sc->a10_bst = rman_get_bustag(sc->a10_mem_res);
	sc->a10_bsh = rman_get_bushandle(sc->a10_mem_res);

	rid = 0;
	sc->a10_irq_res = bus_alloc_resource_any(dev, SYS_RES_IRQ, &rid,
	    RF_ACTIVE | RF_SHAREABLE);
	if (!sc->a10_irq_res) {
		device_printf(dev, "cannot allocate interrupt\n");
		a10_mmc_free_resources(sc) ; 
		//bus_release_resource(dev, SYS_RES_MEMORY, 0, sc->a10_mem_res);
		return (ENXIO);
	}

	if (bus_setup_intr(dev, sc->a10_irq_res, INTR_TYPE_MISC | INTR_MPSAFE,
	    NULL, a10_mmc_intr, sc, &sc->a10_intrhand)) {
		//bus_release_resource(dev, SYS_RES_MEMORY, 0, sc->a10_mem_res);
		//bus_release_resource(dev, SYS_RES_IRQ, 0, sc->a10_irq_res);
		a10_mmc_free_resources(sc) ; 
		device_printf(dev, "cannot setup interrupt handler\n");
		return (ENXIO);
	}
	
	if(a10_mmc_dma_attach(dev) !=0)
	{
		device_printf(dev, "Setting up DMA failed!\n") ; 
		sc->use_dma = 0 ; 
		return (ENOMEM) ; 
	}
	
	sc->use_dma = 1 ; 

	if (a10_clk_mmc_activate(&sc->mod_clk) != 0)
		return (ENXIO);
device_printf(dev, "%s: mod_clk: %d\n", __func__, sc->mod_clk);

	/* Reset controller */
	reg = a10_mmc_read_4(sc, A10_MMC_GCTRL);
	reg |= A10_MMC_SOFT_RESET | A10_MMC_FIFO_RESET | A10_MMC_DMA_RESET;
	a10_mmc_write_4(sc, A10_MMC_GCTRL, reg);
	while(a10_mmc_read_4(sc, A10_MMC_GCTRL) & \
			(A10_MMC_SOFT_RESET | A10_MMC_FIFO_RESET | A10_MMC_DMA_RESET)) {  
		time_left-- ; 
		if(time_left == 0)
			break ; 
	}
	if(time_left == 0) { 

		device_printf(dev, "Reset timedout\n") ; 
		return (ENXIO) ; 
	}
	/* config DMA/Interrupt Trigger threshold */
	//  a10_mmc_write_4(sc, MMC_FTRGL, 0x70008);

	/* Config timeout register */
	a10_mmc_write_4(sc, A10_MMC_TMOUT, 0xffffffff);

	/* Clear interrupt flags */
	a10_mmc_write_4(sc, A10_MMC_RINTR, 0xffffffff);

	a10_mmc_write_4(sc, A10_MMC_DBGC, 0xdeb);
	a10_mmc_write_4(sc, A10_MMC_FUNS, 0xceaa0000);

	sc->a10_host.f_min = 400000;
	sc->a10_host.f_max = 52000000;
	sc->a10_host.host_ocr = MMC_OCR_320_330 | MMC_OCR_330_340;
	sc->a10_host.caps = MMC_CAP_4_BIT_DATA | MMC_CAP_HSPEED;
	sc->a10_host.mode = mode_sd;

	child = device_add_child(dev, "mmc", -1);
	if (child == NULL) {
		device_printf(dev, "attaching MMC bus failed!\n");
		bus_teardown_intr(dev, sc->a10_irq_res, sc->a10_intrhand);
		bus_release_resource(dev, SYS_RES_MEMORY, 0, sc->a10_mem_res);
		bus_release_resource(dev, SYS_RES_IRQ, 0, sc->a10_irq_res);
		return (ENXIO);
	}

	return (device_probe_and_attach(child));

	return (0);
}

static int
a10_mmc_detach(device_t dev)
{
	struct a10_mmc_softc* sc = device_get_softc(dev) ; 
	a10_mmc_free_resources(sc) ; 
	/*TODO :- add code for tearing down DMA */ 
	return (0);
}

static int 
a10_mmc_dma_attach(device_t dev) 
{
	struct a10_mmc_softc* sc = device_get_softc(dev) ; 

	/* Allocate parent DMA tag. */ 
	if(bus_dma_tag_create(bus_get_dma_tag(dev),
				1,
				0,
				BUS_SPACE_MAXADDR,
				BUS_SPACE_MAXADDR,
				NULL,
				NULL,
				BUS_SPACE_MAXSIZE_32BIT,
				BUS_SPACE_UNRESTRICTED,
				BUS_SPACE_MAXSIZE_32BIT,
				0,
				NULL,
				NULL,
				&(sc->dma_sc).a10_mmc_dma_parent_tag)) {  
		device_printf(dev, "Cannot allocate a10_mmc parent DMA tag!\n") ; 
		return (ENOMEM) ; 
	}	
	
	/* Allocate DMA tag for this device.*/ 
	if(bus_dma_tag_create((sc->dma_sc).a10_mmc_dma_parent_tag, 
				1, 
				0,
				BUS_SPACE_MAXADDR,
				BUS_SPACE_MAXADDR,
				NULL,
				NULL,
				1024,
				1,
				BUS_SPACE_MAXSIZE_32BIT,
				0,
				NULL,
				NULL,
				&(sc->dma_sc).a10_mmc_dma_tag)) { 
		device_printf(dev, "Cannot allocate a10_mmc DMA tag!\n") ; 
		return (ENOMEM) ; 	
	}
	
	if(bus_dmamap_create((sc->dma_sc).a10_mmc_dma_tag, 
				0,
				&(sc->dma_sc).a10_mmc_dma_map)) { 
		device_printf(dev, "Cannot allocate a10_mmc DMA map!\n") ; 
		return (ENOMEM) ; 
	}

	uint32_t error = bus_dmamap_load((sc->dma_sc).a10_mmc_dma_tag, 
				(sc->dma_sc).a10_mmc_dma_map,
				(sc->dma_sc).buff,
				BUFF_SIZE,
				a10_mmc_callback,
				(sc->dma_sc).a10_mmc_busaddr,
				BUS_DMA_NOWAIT) ; 
	if(error || (sc->dma_sc).a10_mmc_busaddr == 0) { 
		device_printf(dev, "Cannot load a10_mmc DMA memory!\n") ; 
		a10_mmc_free_resources(sc) ; 
		return (ENOMEM) ; 
	}

	return (0) ; 
	
} 	
static void a10_mmc_callback(void* arg, bus_dma_segment_t* segs, int nseg, int error)
{
	if(error) {  
		printf("Error in mmc card callback function, error code = %u\n", error) ; 
		return;  
	}	
	*(bus_addr_t*)arg = segs[0].ds_addr ; 
}

static void
a10_mmc_free_resources(struct a10_mmc_softc* sc)
{
	if(sc == NULL)
		return ; 

	if(sc->a10_intrhand != NULL)
		bus_teardown_intr(sc->a10_dev, sc->a10_irq_res, sc->a10_intrhand) ; 	

	if(sc->a10_irq_res != NULL)
		bus_release_resource(sc->a10_dev, SYS_RES_IRQ, 0, sc->a10_irq_res) ; 

	if(sc->a10_mem_res != NULL) 
		bus_release_resource(sc->a10_dev, SYS_RES_MEMORY, 0, sc->a10_mem_res) ;
}
		
static void
a10_req_ok(struct a10_mmc_softc *sc)
{
	struct mmc_command *cmd = sc->a10_req->cmd;
	uint32_t resp_status;;

	do{
		resp_status = a10_mmc_read_4(sc, A10_MMC_STAS);
	}while(resp_status & A10_MMC_CARD_DATA_BUSY);

	if (cmd->flags & MMC_RSP_136) {
		cmd->resp[0] = a10_mmc_read_4(sc, A10_MMC_RESP3);
		cmd->resp[1] = a10_mmc_read_4(sc, A10_MMC_RESP2);
		cmd->resp[2] = a10_mmc_read_4(sc, A10_MMC_RESP1);
		cmd->resp[3] = a10_mmc_read_4(sc, A10_MMC_RESP0);
	} else {
		cmd->resp[0] = a10_mmc_read_4(sc, A10_MMC_RESP0);
	}

	sc->a10_req->cmd->error = MMC_ERR_NONE;
	sc->a10_req->done(sc->a10_req);
	sc->a10_req = NULL;
}

static void
a10_req_err(struct a10_mmc_softc *sc)
{
	struct mmc_command *cmd = sc->a10_req->cmd;

	device_printf(sc->a10_dev, "req error\n");
	cmd->error = MMC_ERR_TIMEOUT;
	sc->a10_req->done(sc->a10_req);
	sc->a10_req = NULL;
}

static void
a10_mmc_intr(void *arg)
{
	struct a10_mmc_softc *sc = (struct a10_mmc_softc *)arg;
	struct mmc_command *cmd = sc->a10_req->cmd;
	struct mmc_data *data = cmd->data;
	uint32_t rint = a10_mmc_read_4(sc, A10_MMC_RINTR);
	uint32_t imask = a10_mmc_read_4(sc, A10_MMC_IMASK);

	imask &= ~rint;
	a10_mmc_write_4(sc, A10_MMC_IMASK, imask);
	a10_mmc_write_4(sc, A10_MMC_RINTR, rint);

	if(sc->a10_req == NULL){
		device_printf(sc->a10_dev, "req == NULL, rint: 0x%08X\n", rint);
	}

	if(rint & A10_MMC_INT_ERR_BIT){
		device_printf(sc->a10_dev, "error rint: 0x%08X\n", rint);
		a10_req_err(sc);
		return;
	}

	if(!data && (rint & A10_MMC_CMD_DONE)){
		a10_req_ok(sc);
		return;
	}

	if(data && (rint & A10_MMC_DATA_OVER)){
		a10_req_ok(sc);
		return;
	}

	if(data->flags & MMC_DATA_READ){
		uint32_t *buff = (uint32_t*)data->data;
		for (uint32_t i = 0; i < (data->len >> 2); i++) {
			while(a10_mmc_read_4(sc, A10_MMC_STAS) & A10_MMC_FIFO_EMPTY);
			buff[i] = a10_mmc_read_4(sc, A10_MMC_FIFO);
		}
	}

	if((cmd->data->flags & MMC_DATA_WRITE) && (rint & A10_MMC_TX_DATA_REQ)){
		uint32_t *buff = (uint32_t*)data->data;
		for (uint32_t i = 0; i < (data->len >> 2); i++) {
			while(a10_mmc_read_4(sc, A10_MMC_STAS) & A10_MMC_FIFO_FULL);
			a10_mmc_write_4(sc, A10_MMC_FIFO, buff[i]);
		}
	}

	return;	
}

static int
a10_mmc_request(device_t bus, device_t child, struct mmc_request *req)
{
	struct a10_mmc_softc *sc = device_get_softc(bus);
	struct mmc_command *cmd = req->cmd;
	uint32_t cmdreg = A10_MMC_START;
	uint32_t imask = A10_MMC_CMD_DONE | A10_MMC_INT_ERR_BIT;

	a10_mmc_lock(sc);
	if (sc->a10_req){
		a10_mmc_unlock(sc);
		return (EBUSY);
	}

	sc->a10_req = req;

	if (cmd->opcode == MMC_GO_IDLE_STATE)
		cmdreg |= A10_MMC_SEND_INIT_SEQ;
	if (cmd->flags & MMC_RSP_PRESENT)
		cmdreg |= A10_MMC_RESP_EXP;
	if (cmd->flags & MMC_RSP_136)
		cmdreg |= A10_MMC_LONG_RESP;
	if (cmd->flags & MMC_RSP_CRC)
		cmdreg |= A10_MMC_CHECK_RESP_CRC;

	if (cmd->data) {
		cmdreg |= A10_MMC_DATA_EXP | A10_MMC_WAIT_PREOVER;
		imask |= A10_MMC_DATA_OVER;
		if (cmd->data->flags & MMC_DATA_WRITE){
			cmdreg |= A10_MMC_WRITE;
			imask |= A10_MMC_TX_DATA_REQ;
		}else{
			imask |= A10_MMC_RX_DATA_REQ;
		}

		a10_mmc_write_4(sc, A10_MMC_BLKSZ, cmd->data->len);
		a10_mmc_write_4(sc, A10_MMC_BCNTR, cmd->data->len);

		/* Choose access by AHB */
		a10_mmc_write_4(sc, A10_MMC_GCTRL, 
				a10_mmc_read_4(sc, A10_MMC_GCTRL)|A10_MMC_ACCESS_BY_AHB);
	}

	if (cmd->flags & MMC_RSP_BUSY) {
		imask |= A10_MMC_DATA_TIMEOUT;
	}

	/* Enable interrupts and set IMASK */
	a10_mmc_write_4(sc, A10_MMC_IMASK, imask);
    a10_mmc_write_4(sc, A10_MMC_GCTRL, 
			a10_mmc_read_4(sc, A10_MMC_GCTRL)|A10_MMC_INT_ENABLE);

	a10_mmc_write_4(sc, A10_MMC_CARG, cmd->arg);
	a10_mmc_write_4(sc, A10_MMC_CMDR, cmdreg|cmd->opcode);

	a10_mmc_unlock(sc);

	return 0;
}

static int
a10_mmc_read_ivar(device_t bus, device_t child, int which, 
    uintptr_t *result)
{
	struct a10_mmc_softc *sc = device_get_softc(bus);

	switch (which) {
	default:
		return (EINVAL);
	case MMCBR_IVAR_BUS_MODE:
		*(int *)result = sc->a10_host.ios.bus_mode;
		break;
	case MMCBR_IVAR_BUS_WIDTH:
		*(int *)result = sc->a10_host.ios.bus_width;
		break;
	case MMCBR_IVAR_CHIP_SELECT:
		*(int *)result = sc->a10_host.ios.chip_select;
		break;
	case MMCBR_IVAR_CLOCK:
		*(int *)result = sc->a10_host.ios.clock;
		break;
	case MMCBR_IVAR_F_MIN:
		*(int *)result = sc->a10_host.f_min;
		break;
	case MMCBR_IVAR_F_MAX:
		*(int *)result = sc->a10_host.f_max;
		break;
	case MMCBR_IVAR_HOST_OCR:
		*(int *)result = sc->a10_host.host_ocr;
		break;
	case MMCBR_IVAR_MODE:
		*(int *)result = sc->a10_host.mode;
		break;
	case MMCBR_IVAR_OCR:
		*(int *)result = sc->a10_host.ocr;
		break;
	case MMCBR_IVAR_POWER_MODE:
		*(int *)result = sc->a10_host.ios.power_mode;
		break;
	case MMCBR_IVAR_VDD:
		*(int *)result = sc->a10_host.ios.vdd;
		break;
	case MMCBR_IVAR_CAPS:
		*(int *)result = sc->a10_host.caps;
		break;
	case MMCBR_IVAR_MAX_DATA:
		*(int *)result = 1;
		break;
	}

	return (0);
}

static int
a10_mmc_write_ivar(device_t bus, device_t child, int which,
    uintptr_t value)
{
	struct a10_mmc_softc *sc = device_get_softc(bus);

	switch (which) {
	default:
		return (EINVAL);
	case MMCBR_IVAR_BUS_MODE:
		sc->a10_host.ios.bus_mode = value;
		break;
	case MMCBR_IVAR_BUS_WIDTH:
		sc->a10_host.ios.bus_width = value;
		break;
	case MMCBR_IVAR_CHIP_SELECT:
		sc->a10_host.ios.chip_select = value;
		break;
	case MMCBR_IVAR_CLOCK:
		sc->a10_host.ios.clock = value;
		break;
	case MMCBR_IVAR_MODE:
		sc->a10_host.mode = value;
		break;
	case MMCBR_IVAR_OCR:
		sc->a10_host.ocr = value;
		break;
	case MMCBR_IVAR_POWER_MODE:
		sc->a10_host.ios.power_mode = value;
		break;
	case MMCBR_IVAR_VDD:
		sc->a10_host.ios.vdd = value;
		break;
	/* These are read-only */
	case MMCBR_IVAR_CAPS:
	case MMCBR_IVAR_HOST_OCR:
	case MMCBR_IVAR_F_MIN:
	case MMCBR_IVAR_F_MAX:
	case MMCBR_IVAR_MAX_DATA:
		return (EINVAL);
	}
	return (0);
}

static int
a10_mmc_update_ios(device_t bus, device_t child)
{
	struct a10_mmc_softc *sc = device_get_softc(bus);
	struct mmc_ios *ios = &sc->a10_host.ios;
	uint32_t clkdiv = 0;
	uint32_t cmdreg = A10_MMC_START | A10_MMC_UPCLK_ONLY | A10_MMC_WAIT_PREOVER;
	uint32_t rval = a10_mmc_read_4(sc, A10_MMC_CLKCR);

	/* Change clock first */
	clkdiv = (sc->mod_clk + (ios->clock >> 1))/ios->clock/2;

	if (ios->clock) {
		/* Disable clock */
		rval &= ~A10_MMC_CARD_CLK_ON;
		a10_mmc_write_4(sc, A10_MMC_CLKCR, rval);

		a10_mmc_write_4(sc, A10_MMC_CMDR, cmdreg);
		while(a10_mmc_read_4(sc, A10_MMC_CMDR) & A10_MMC_START);

		/* Change divider */
		rval &= ~(0xFF);
		rval |= clkdiv;
		a10_mmc_write_4(sc, A10_MMC_CLKCR, rval);

		a10_mmc_write_4(sc, A10_MMC_CMDR, cmdreg);
		while(a10_mmc_read_4(sc, A10_MMC_CMDR) & A10_MMC_START);

		/* Enable clock */
		rval |= A10_MMC_CARD_CLK_ON;
		a10_mmc_write_4(sc, A10_MMC_CLKCR, rval);

		a10_mmc_write_4(sc, A10_MMC_CMDR, cmdreg);
		while(a10_mmc_read_4(sc, A10_MMC_CMDR) & A10_MMC_START);
	}

	/* Set the bus width */
	switch (ios->bus_width) {
		case bus_width_1:
			a10_mmc_write_4(sc, A10_MMC_WIDTH, A10_MMC_WIDTH1);
			break;
		case bus_width_4:
			a10_mmc_write_4(sc, A10_MMC_WIDTH, A10_MMC_WIDTH4);
			break;
		case bus_width_8:
			a10_mmc_write_4(sc, A10_MMC_WIDTH, A10_MMC_WIDTH8);
			break;
	}

	return (0);
}

static int
a10_mmc_get_ro(device_t bus, device_t child)
{

	return (0);
}

static int
a10_mmc_acquire_host(device_t bus, device_t child)
{
	struct a10_mmc_softc *sc = device_get_softc(bus);
	int error = 0;

	a10_mmc_lock(sc);
	while (sc->a10_bus_busy)
		error = mtx_sleep(sc, &sc->a10_mtx, PZERO, "mmcah", 0);

	sc->a10_bus_busy++;
	a10_mmc_unlock(sc);
	return (error);
}

static int
a10_mmc_release_host(device_t bus, device_t child)
{
	struct a10_mmc_softc *sc = device_get_softc(bus);

	a10_mmc_lock(sc);
	sc->a10_bus_busy--;
	wakeup(sc);
	a10_mmc_unlock(sc);
	return (0);
}

static device_method_t a10_mmc_methods[] = {
	/* Device interface */
	DEVMETHOD(device_probe,		a10_mmc_probe),
	DEVMETHOD(device_attach,	a10_mmc_attach),
	DEVMETHOD(device_detach,	a10_mmc_detach),

	/* Bus interface */
	DEVMETHOD(bus_read_ivar,	a10_mmc_read_ivar),
	DEVMETHOD(bus_write_ivar,	a10_mmc_write_ivar),
	DEVMETHOD(bus_print_child,	bus_generic_print_child),

	/* MMC bridge interface */
	DEVMETHOD(mmcbr_update_ios,	a10_mmc_update_ios),
	DEVMETHOD(mmcbr_request,	a10_mmc_request),
	DEVMETHOD(mmcbr_get_ro,		a10_mmc_get_ro),
	DEVMETHOD(mmcbr_acquire_host,	a10_mmc_acquire_host),
	DEVMETHOD(mmcbr_release_host,	a10_mmc_release_host),

	DEVMETHOD_END
};

static devclass_t a10_mmc_devclass;

static driver_t a10_mmc_driver = {
	"a10_mmc",
	a10_mmc_methods,
	sizeof(struct a10_mmc_softc),
};

DRIVER_MODULE(a10_mmc, simplebus, a10_mmc_driver, a10_mmc_devclass, 0, 0);
