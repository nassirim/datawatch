#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dlfcn.h>
#include <inttypes.h>
#include <sys/types.h>
#include "dwhooks.h"

const uintptr_t DW_MASK = ~(65535ULL << 48);

void print_pointer(uintptr_t addr, const char *msg) {

	uintptr_t i = (1ULL << (sizeof(addr)*8-1));
	int j = 16;

	printf("%20s: %20p  Binary: ", msg, (int *)addr);
	for(; i; i >>= 1) {
		if (j == 0) printf(" :: "); // as seperator between 16 MSB and 48 LSB
		printf("%d",(addr & i)!=0);
		j--;
	}

	printf("\n");
}


/* Insert the tag into the 16 MSB of the pointer*/
uintptr_t taint(uintptr_t p, uint16_t tag_data) 
{
  p = (uintptr_t)(((uintptr_t)p & DW_MASK) | ((uintptr_t)tag_data << 48));

  return(p);
}

/* Remove the tag from the 16 MSB of the pointer*/
uintptr_t untaint(uintptr_t p) 
{
	p = (void *)(((intptr_t)p << 16) >> 16);

  return(p);
}

int main(int argc, char* argv[]) {

  dw_init();

  int *ptr = malloc(sizeof(int));
  // ptr pointer was tagged in the malloc hook.


	uint16_t tag_data = 12345; // This is the tag to store in the top 16 bits 
  print_pointer(ptr, "After Malloc:");
  ptr = untaint(ptr);
  print_pointer(ptr, "After untaint:");

  // Access is authorized as the address is untainted
  *ptr = 20;

  /* 
   * Pass an ordinary (untainted) pointer to a system call
   */
  printf("The value of ptr is : %p\n", ptr);
  int nW = write(1, ptr, sizeof(int));
  printf("Write SYSCALL: number of bytes written : %d\n", nW);

  ptr =  taint(ptr, tag_data);

  /* 
   * Pass a tainted pointer to a system call
   */
  printf("The value of ptr is : %p\n", ptr);
  nW = write(1, ptr, sizeof(int));
  printf("Write SYSCALL: number of bytes written : %d\n", nW);
  
  // Access will generate a SIGSEGV as the address is tainted.
  *ptr = 20;

  free(ptr);

  printf("PID:%ld\n",(long)getpid());
}
