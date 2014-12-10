

#include <stdio.h>
#include "dma.h"
#include <sys/time.h>	// gettimeofday() for debug
#include <stdlib.h>


struct timeval ts,te;
			

int main(void)
{

	#define DATA_LEN 16
	#define DATA_OFFSET 20

	char* hw_mem = dma_malloc(4097);
	char* sw_mem = malloc(1000);

	printf("DMA test\n");

	int x;

	dma_status();

	for (x=0; x< DATA_LEN; x++)
	{
		hw_mem[x] = x;
		sw_mem[x] = x;
	}
	
	for (x=0; x < DATA_LEN + DATA_OFFSET + 4; x++){ if (x%8==0) printf("\n"); printf("%2d ", hw_mem[x]); }	printf("\n");

	printf("Do dma_copy()\n");

	gettimeofday(&ts,NULL);

	dma_copy(&hw_mem[0], &hw_mem[DATA_OFFSET], DATA_LEN);

	gettimeofday(&te,NULL);

	for (x=0; x < DATA_LEN + DATA_OFFSET + 4; x++){ if (x%8==0) printf("\n"); printf("%2d ", hw_mem[x]); }	printf("\n");
	dma_status();

	printf("dma took %dus for length %d\n", 1000000 * te.tv_sec + te.tv_usec - (1000000 * ts.tv_sec + ts.tv_usec),DATA_LEN);

	dma_free(hw_mem);
	free(sw_mem);

	printf("Finished DMA test\n");

	return 0;
}
