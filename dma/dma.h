#ifndef DMA_H
#define DMA_H



#define DMA_CHAN_SIZE           0x100
#define DMA_CHAN_MIN            0
#define DMA_CHAN_MAX            14
#define DMA_CHAN_DEFAULT        14

#if 1
#define DMA_BASE                0x20007000
#else
#define DMA_BASE				0x7E007000	// BCM2835 ARM Peripherals.pdf says the address is this
#endif

#define DMA_LEN                 DMA_CHAN_SIZE * (DMA_CHAN_MAX+1)

#define DMA_CS 					(0x00/4)
#define DMA_CONBLK_AD 			(0x04/4)
#define DMA_TI 					(0x08/4)
#define DMA_SRC 				(0x0C/4)
#define DMA_DEST 				(0x10/4)
#define DMA_TLEN 				(0x14/4)
#define DMA_STRIDE				(0x18/4)
#define DMA_NEXT				(0x1C/4)
#define DMA_DEBUG 				(0x20/4)

#define DMA_TI_NO_WIDE_BURSTS 					(1<<26)
#define DMA_TI_D_DREQ 							(0x00000040)
#define DMA_TI_PER_MAP(x) 						((x)<<16)
#define DMA_TI_SRC_IGNORE						(0x00000800)	// ???
#define DMA_TI_SRC_DREQ							(0x00000400)	// 1 DREQ selected by PERMAP will gate the src read 
#define DMA_TI_SRC_WIDTH						(0x00000200)	// 1 = 128-bit read width, 0 = 32-bit
#define DMA_TI_SRC_INC							(0x00000100)	// 1 addr+=4 if SRC_WIDTH=0 else addr+=32, 0 Src addr does not inc
#define DMA_TI_DST_IGNORE						(0x00000080)	// 0 write data to dest
#define DMA_TI_DST_DREQ							(0x00000040)	// 1 DREQ selected by PERMAP will gate the dest writes 
#define DMA_TI_DST_WIDTH						(0x00000020)	// 1 = 128-bit write width, 0 = 32-bit
#define DMA_TI_DST_INC							(0x00000010)	// 1 addr+=4 if DEST_WIDTH=0 else addr+=32, 0 Dest addr does not inc
#define DMA_TI_WAIT_RESP 						(0x00000008)
#define DMA_TI_2D		 						(0x00000002)
#define DMA_TI_INT		 						(0x00000001)

#define DMA_CS_RESET 							(0x80000000)
#define DMA_CS_ABORT 							(0x40000000)
#define DMA_CS_DISDEBUG 						(0x20000000)
#define DMA_CS_WAIT_FOR_OUTSTANDING_WRITES		(0x10000000)
#define DMA_CS_PANIC_PRIORITY			 		(0x00F00000)
#define DMA_CS_PRIORITY 						(0x000F0000)
#define DMA_CS_ERROR							(0x00000100)
#define DMA_CS_WAITING_FOR_OUTSTANDING_WRITES	(0x00000040)
#define DMA_CS_DREQ_STOPS_DMA 					(0x00000020)
#define DMA_CS_PAUSED 							(0x00000010)
#define DMA_CS_DREQ 							(0x00000008)
#define DMA_CS_INT 								(0x00000004)
#define DMA_CS_END 								(0x00000002)
#define DMA_CS_ACTIVE 							(0x00000001)

#define DMA_DBG_LITE							(0x10000000)
#define DMA_DBG_VERSION							(0x0E000000)
#define DMA_DBG_STATE							(0x01FF0000)
#define DMA_DBG_ID								(0x0000FF00)
#define DMA_DBG_OUTSTANDING_WRITES				(0x000000F0)
#define DMA_DBG_READ_ERROR						(0x00000004)
#define DMA_DBG_FIFO_ERROR						(0x00000002)
#define DMA_DBG_READ_LAST_NOT_SET_ERROR			(0x00000001)

void dma_status(void);

void dma_copy(void* from, void* to, unsigned int len);

void dma_WaitComplete(unsigned int type);

void* dma_malloc(unsigned int bytes);

void dma_free(void*);

#endif

