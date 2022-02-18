#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dlfcn.h>
#include "dwhooks.h"


int main(int argc, char* argv[]) {

  dw_init();
  int *m3 = malloc(16*sizeof(int));
  // m3 pointer was tagged


//  *m3 = 20;

  printf("Allocation done\n");
  printf("A tagged pointer passed to the printf system call :%i\n",*m3);

  free(m3);

  printf("PID:%ld",(long)getpid());
}
