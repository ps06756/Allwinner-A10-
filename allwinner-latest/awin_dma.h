#ifndef A10_DMA_H 
#define A10_DMA_H 
/* This file will contain the generic data structures used to manipulate DMA controller for allwinner board. */ 

/* Structures used for DMA access. */ 

struct a10_dma_controller { 
 	uint32_t (*dma_get_config)(void*),
	void (*dma_set_config)(void*, uint32_t conf)
} ; 
	

struct a10_dma_softc { 
	device_t dev ; 
	bus_space_tag_t bst ; 
	bus_space_handle_t bsh 
	bus_dma_tag_t dmat ; 
} ; 

enum a10_dma_type { 
	NDMA, 	
	DDMA
} ; 

/* module base address. */ 
#define DMA (0x01C02000) 


/* These are macros of Normal DMA. */ 
#define DMA_IRQ_EN_REG (0x0000) 
#define DMA_IRQ_PEND_STA_REG (0x0004)
#define NDMA_CTRL_REG(n) (0x100 + ((n)*0x20))
#define NDMA_SRC_ADDR_REG(n) (0x100 + ((n)*0x20) + 4)
#define NDMA_DEST_ADDR_REG(n) (0x100 + ((n)*0x20) + 8)
#define NDMA_BC_REG(n) (0x100 + ((n)*0x20) + 0x0C ) 

/* These aee macros of Dedicated DMA */ 

#define DDMA_CFG_REG(n) (0x300 + ((n)*0x20) ) 
#define DDMA_SRC_START_ADDR_REG(n) (0x300 + ((n)*0x20) + 4) 
#define DDMA_DEST_START_ADDR_REG(n) (0x300 + ((n)*0x20) + 8) 
#define DDMA_BC_REG(n)  (0x300 + ((n)*0x20 ) + 0x0C ) 
#define DDMA_PARA_REG(n)  (0x300 + ((n)*0x20) + 0x18) 


/* Macros to manipulate DMA_IRQ_EN_REG */ 

#define DDMA_IRQ_FULL_ENABLE(n) (1 << (17 + (2*(n))))
#define DDMA_IRQ_FULL_DISABLE(n) (~(1 << (17 + (2*(n)))))
#define DDMA_IRQ_HALF_ENABLE(n) (1 << (16 + (2*(n)))) 
#define DDMA_IRQ_HALF_DISABLE(n) (~(1 << (16 + (2*(n))))) 

#define NDMA_IRQ_FULL_ENABLE(n) (1 << (1 + (2*(n)))) 
#define NDMA_IRQ_FULL_DISABLE(n) (~(1 << (1 + (2*(n)))))
#define NDMA_IRQ_HALF_ENABLE(n) (1 << (16 + (2*(n)))) 
#define NDMA_IRQ_HALF_DISABLE(n) (~(1 << (16 + 2*(n)))) 

/* Some macros for reading and writing from the registers.(to be used with softc */ 

#define DMA_READ(sc, reg) \ 
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg)) 
#define DMA_WRITE(sc, reg, val) \ 
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val)) 
#endif 