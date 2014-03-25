

#include <stdio.h>
#include "dma.h"
#include <sys/time.h>	// gettimeofday() for debug
#include <stdlib.h>

int main(void)
{
	char* hw_mem = dma_malloc(1000);
	char* sw_mem = malloc(1000);

	printf("DMA test\n");




	dma_free(hw_mem);
	free(sw_mem);

	printf("Finished\n");

	return 0;
}
