#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>		// types
#include <unistd.h>		// usleep()
#include <sys/mman.h>	// mmap()
#include <sys/time.h>	// gettimeofday() for debug
#include <fcntl.h>

#include "dma.h"

//---------------------------------------------

#define PRINT_DMA_MSG(...) 		printf(__VA_ARGS__)

#ifndef PRINT_DMA_MSG
#define PRINT_DMA_MSG(...)
#endif

#define PAGE_SIZE               4096
#define PAGE_SHIFT              12

//----------------------------------------------

typedef struct {
        unsigned int physaddr;
		unsigned int numContiguous;
} page_map_t;

// The Control Block (CB) is copied by the dma controller into its own memory
typedef struct {
    unsigned int TI;
    unsigned int SOURCE_AD;
    unsigned int DEST_AD;
    unsigned int LENGTH;
    unsigned int STRIDE;
    unsigned int NEXTCONBK;
} dma_cb_t;

//----------------------------------------------

static dma_cb_t*			cb_base;						// This will hold the pointer to the program memory used for passing parameters to the dma controller
static uint32_t 			physical_cb_base;				// This is the physical address for the 'virtual' cb_base memory. 

static volatile uint32_t* 	dma_reg 	= NULL;				// A pointer to the DMA controller
static int 					dma_chan 	= DMA_CHAN_DEFAULT;	// The DMA controller ID
static uint32_t*			dma_memory = NULL;	


static page_map_t* 			dma_memory_map;							//This holds the virtual <=> physical mappings
static uint32_t				num_pages;

//----------------------------------------------

static uint32_t mem_virt_to_phys(uint32_t* virt, uint32_t* numContiguous)
{
   	uint32_t offset = (uint32_t)virt - (uint32_t)dma_memory;

	if (!dma_memory || (virt > (uint32_t *)cb_base || virt < dma_memory))	//cb_base is the last region in the mmap'ed space
	{
		PRINT_DMA_MSG( "DMA invalid location %p, offset %d, page %d", virt, offset, (offset >> PAGE_SHIFT));
		if (numContiguous) *numContiguous = 0;
		return 0;
	}
	if (numContiguous) *numContiguous = dma_memory_map[offset >> PAGE_SHIFT].numContiguous;

	PRINT_DMA_MSG( "DMA mem_virt_to_phys %p => 0x%X, offset %d, page %d\n", virt, dma_memory_map[offset >> PAGE_SHIFT].physaddr + (offset & ~(PAGE_SIZE-1)), offset, (offset >> PAGE_SHIFT));

	return dma_memory_map[offset >> PAGE_SHIFT].physaddr + (offset & ~(PAGE_SIZE-1));
}

static void * map_peripheral(uint32_t base, uint32_t len)
{
    int fd = open("/dev/mem", O_RDWR);
    void * vaddr;

    if (fd < 0)
	{
            PRINT_DMA_MSG("%d, Failed to open /dev/mem: %m", __LINE__);
    		return NULL;
	}
	else
	{
		vaddr = mmap(NULL, len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, base);

		if (vaddr == MAP_FAILED)
		{
			PRINT_DMA_MSG("Failed to map peripheral at 0x%08x: %m\n", base);
			return NULL;
		}

		close(fd);
		return vaddr;
	}

	PRINT_DMA_MSG("Cannot get DMA Controller");

	return NULL;
}


static void make_pagemap(void* virtcached)
{
    int i,j, fd, memfd, pid;
    char pagemap_fn[64];

	PRINT_DMA_MSG("make_pagemap()\n");

	dma_memory_map = malloc(num_pages* sizeof(page_map_t) );
    if (dma_memory_map == 0)
            PRINT_DMA_MSG("Failed to malloc page_map: %m\n");
    memfd = open("/dev/mem", O_RDWR);
    if (memfd < 0)
            PRINT_DMA_MSG("Failed to open /dev/mem: %m\n");
    pid = getpid();
    sprintf(pagemap_fn, "/proc/%d/pagemap", pid);
    fd = open(pagemap_fn, O_RDONLY);
    if (fd < 0)
            PRINT_DMA_MSG("Failed to open %s: %m\n", pagemap_fn);
    if (lseek(fd, (uint32_t)(size_t)virtcached >> 9, SEEK_SET) !=
                                            (uint32_t)(size_t)virtcached >> 9) {
            PRINT_DMA_MSG("Failed to seek on %s: %m\n", pagemap_fn);
    }
    for (i = 0; i < num_pages; i++) 
	{
        uint64_t pfn;
        if (read(fd, &pfn, sizeof(pfn)) != sizeof(pfn)) PRINT_DMA_MSG("Failed to read %s: %m\n", pagemap_fn);
        if (((pfn >> 55) & 0x1bf) != 0x10c) 			PRINT_DMA_MSG("Page %d not present (pfn 0x%016llx)\n", i, pfn);
        
		dma_memory_map[i].physaddr = (uint32_t)pfn << PAGE_SHIFT | 0x40000000;
		dma_memory_map[i].numContiguous = 0;
	
		//record if last pages preceed this one in physical memory 
		if (i > 0 && dma_memory_map[i-1].physaddr + PAGE_SIZE == dma_memory_map[i].physaddr)
		{ 
			dma_memory_map[i-1].numContiguous = 1;
			j = i - 1;
			while (j > 0)
			{
				j--;
				if (dma_memory_map[j].numContiguous)		//this will be 0 if the next blocks physical memory is not contiguous
					dma_memory_map[j].numContiguous++;
				else break; 				
			}
		}    

		if ( i<10 || i%1000 == 0 || i == num_pages-1) PRINT_DMA_MSG("DMA %4d, p addr 0x%X, v addr 0x%X\n",i, dma_memory_map[i].physaddr, (unsigned char*)dma_memory + i * PAGE_SIZE); 

		if (mmap((unsigned char*)dma_memory + i * PAGE_SIZE, PAGE_SIZE, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_SHARED|MAP_FIXED|MAP_LOCKED|MAP_NORESERVE,
                memfd, (uint32_t)pfn << PAGE_SHIFT | 0x40000000) != (unsigned char*)dma_memory + i * PAGE_SIZE) 
		{
			PRINT_DMA_MSG("Failed to create uncached map of page %d at %p\n", i, (unsigned char*)dma_memory + i * PAGE_SIZE);
			//break;
        }
    }

    close(fd);
    close(memfd);

	//if you want to find out how many blocks are contiguous then include the following
#if 0
	for (i=0; i < 1; i++)
	{
		PRINT_DMA_MSG("map %4d %p %d", i, dma_memory_map[i].physaddr, dma_memory_map[i].numContiguous);
	}
#endif

	//PRINT_DMA_MSG("Allocated DMA memory\n"); 
}


void dma_WaitComplete(unsigned int type)
{
//		PRINT_DMA_MSG( "dma_WaitForComplete(%d), CS 0x%X, BLK 0x%X, DBG 0x%X", type, dma_reg[DMA_CS], dma_reg[DMA_CONBLK_AD], dma_reg[DMA_DEBUG]);
		int x;
		for (x=0; x< 20; x++)
		{
			if(!(dma_reg[DMA_CS] & DMA_CS_ACTIVE))	break;
#if 0
			PRINT_DMA_MSG("%3d dma_WaitForComplete(%d), CS 0x%X, BLK 0x%X, TI 0x%X, SRC 0x%X, DST 0x%X, DBG 0x%X"\n,
				x, type, dma_reg[DMA_CS], dma_reg[DMA_CONBLK_AD], dma_reg[DMA_TI], dma_reg[DMA_SRC], dma_reg[DMA_DEST], dma_reg[DMA_DEBUG]);
#endif
			usleep(1);
		}
		dma_reg[DMA_CS] |= DMA_CS_END | DMA_CS_RESET;
}

void* dma_malloc(unsigned int size)
{
	dma_reg = map_peripheral(DMA_BASE, DMA_LEN);
	
	void* virtcached;	//used as a 'temporary' store so that we don't mmap over other valid memory already allocated in the application

	if (!dma_reg) // failed to get DMA controller memory region
	{
		return NULL;
	}

	// Need to get enough space for what is requested + one page to hold the dma control block (cb_base)
	num_pages = ( (size + (PAGE_SIZE-1)) >> PAGE_SHIFT) + 1;	
	
	PRINT_DMA_MSG("Creating %d pages\n", num_pages);

	virtcached = mmap(NULL, num_pages * PAGE_SIZE, PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS|MAP_NORESERVE|MAP_LOCKED, -1, 0);

	if (virtcached == MAP_FAILED){
 		PRINT_DMA_MSG("Failed to mmap for cached pages: %m\n");
		return NULL;
	}

	if ((unsigned long)virtcached & (PAGE_SIZE-1))
	{
			PRINT_DMA_MSG("virtcached is not page aligned\n");
			virtcached = (char*)(((unsigned long)virtcached & ~(PAGE_SIZE-1)) + PAGE_SIZE);	//align virtcached
	}   	

	//force linux to allocate
	memset(virtcached, 0, num_pages * PAGE_SIZE);
		

	dma_memory = mmap(NULL, num_pages * PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS|MAP_NORESERVE|MAP_LOCKED, -1, 0);
	
	//setup page mapping and link physical memory to virtual;
	make_pagemap(virtcached);
	
	//set the cb_base to be the last page
	cb_base = (dma_cb_t*)((char*)dma_memory + PAGE_SIZE * (num_pages-1));
	
	PRINT_DMA_MSG("dma_memory %p, cb_base %p\n",dma_memory,cb_base);
	
	// get the physical address we are going to use for DMA Control Blocks
	physical_cb_base = mem_virt_to_phys((uint32_t*)cb_base, 0);
	cb_base->NEXTCONBK = 0;		//DMA will stop when its current block is 0 else it will move onto this block 

	//move DMA pointer to desired channel
	dma_reg += dma_chan * DMA_CHAN_SIZE / sizeof(uint32_t);

	// reset DMA controller
	dma_reg[DMA_CS] |= DMA_CS_RESET;

    usleep(10);

	dma_reg[DMA_CS] |=  DMA_CS_INT | DMA_CS_END;
	dma_reg[DMA_CONBLK_AD] = physical_cb_base;
    dma_reg[DMA_DEBUG] = 7; // clear debug error flags
    dma_reg[DMA_CS] = 0x10880001;        // go, mid priority, wait for outstanding writes

	PRINT_DMA_MSG("Hardware DMA Initialized. CB addr %p, CS 0x%X\n", physical_cb_base, dma_reg[DMA_CS]);

	return dma_memory;
}

void dma_copy(void* from, void* to, uint32_t len)
{
	dma_WaitComplete(0);

	uint32_t numBoundaryCrossingsSrc = (((uint32_t)from + len) >> PAGE_SHIFT) - ((uint32_t)from >> PAGE_SHIFT);
	uint32_t numBoundaryCrossingsDst = (((uint32_t)to + len) >> PAGE_SHIFT) - ((uint32_t)to >> PAGE_SHIFT);

	uint32_t numContiguousSrc, numContiguousDst;

	uint32_t physicalSrc = mem_virt_to_phys(from, &numContiguousSrc);
	uint32_t physicalDest = mem_virt_to_phys(to, &numContiguousDst);

	// Can we do a transfer in one hit?
	if (numContiguousSrc >= numBoundaryCrossingsSrc && numContiguousDst >= numBoundaryCrossingsDst && physicalSrc && physicalDest)
	{
		cb_base->TI = DMA_TI_NO_WIDE_BURSTS | DMA_TI_WAIT_RESP | DMA_TI_DST_INC | DMA_TI_SRC_INC;
 		cb_base->SOURCE_AD = physicalSrc;
		cb_base->DEST_AD = physicalDest;
		cb_base->LENGTH = len;
		cb_base->STRIDE = 0;

		//start the transfer
		dma_reg[DMA_CONBLK_AD] = (uint32_t)physical_cb_base;
		dma_reg[DMA_CS] |= DMA_CS_ACTIVE;
	}
	else
	{
		PRINT_DMA_MSG("%d dma panic! %p =>%p, length %d. %d %d %d %d\n", __LINE__, physicalSrc, physicalDest, len, numBoundaryCrossingsSrc, numBoundaryCrossingsDst, numContiguousSrc, numContiguousDst); 
		memcpy(to, from, len);
	}
}

void dma_free(void* dma_memory)
{
	if (dma_reg)
	{
		dma_reg[DMA_CS] |= DMA_CS_RESET;
	}

	if (dma_memory) munmap(dma_memory, num_pages * PAGE_SIZE);
	dma_memory = NULL;
}	

void dma_status(void)
{
	printf("CS: Active %d, END %d, ABORT %d, ERROR %d, PAUSED %d, DREQ %d\n", 
		(dma_reg[DMA_CS] & DMA_CS_ACTIVE), 
		(dma_reg[DMA_CS] & DMA_CS_END)>>1, 
		(dma_reg[DMA_CS] & DMA_CS_ABORT)>>30, 
		(dma_reg[DMA_CS] & DMA_CS_ERROR)>>8, 
		(dma_reg[DMA_CS] & DMA_CS_PAUSED)>>4, 
		(dma_reg[DMA_CS] & DMA_CS_DREQ)>>3);

	printf("TI: INT 0x%X, 2D %d, WAIT_RESP %d\n", 
		(dma_reg[DMA_TI] & DMA_TI_INT)>>0, 
		(dma_reg[DMA_TI] & DMA_TI_2D)>>1, 
		(dma_reg[DMA_TI] & DMA_TI_WAIT_RESP)>>3);

	if (dma_reg[DMA_TI] & DMA_TI_2D)
	{
		printf("Src 0x%X, Dest 0x%X, Len %d x %d, Stride %d x %d\n", 
			(dma_reg[DMA_SRC], dma_reg[DMA_DEST]), 
			(dma_reg[DMA_TLEN] & 0xffff0000)>>16, 
			(dma_reg[DMA_TLEN] & 0x0000ffff),
			(dma_reg[DMA_STRIDE] & 0xffff0000)>>16, 
			(dma_reg[DMA_STRIDE] & 0x0000ffff));
	}
	else
	{
		printf("Src 0x%X, Dest 0x%X, Len %u, Stride %u\n", 
			(dma_reg[DMA_SRC], dma_reg[DMA_DEST]), 
			(dma_reg[DMA_TLEN]),
			(dma_reg[DMA_STRIDE]));
	}

	printf("Current CB 0x%X, Next CB 0x%X \n", 
		(dma_reg[DMA_CONBLK_AD]), 
		(dma_reg[DMA_NEXT]));
	printf("___________________________________________\n\n");
}
