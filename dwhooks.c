#define _GNU_SOURCE

#include <malloc.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <pthread.h>
#include <ucontext.h>
#include <stdio.h>
#include <stdint.h>

#include <execinfo.h>

#include <execinfo.h>
#include <unistd.h>

typedef __uint64_t uint64_t;
#define MAX_MALLOCS 0xFF0
#define START_MALLOC 0x0000
#define DW_TAG 0xC0000000000000
#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

/* Prototypes for our hooks */
static void *dw_malloc_hook(size_t, const void *);
static void dw_free_hook (void*, const void *);


/* Variables to save original hooks */
static void *(*old_malloc_hook)(size_t, const void *);
static void (*old_free_hook) (void*, const void *);

// where we store the allocated size
static size_t *sizes;
static void* *original_address;
static void* *return_address;

static volatile uint16_t dw_TAG; // A constant tag to store in the top 16 bits of any new pointer
static volatile uintptr_t dw_MASK;

// keeps track of where we are in the array
static volatile int count, all_count, free_count;

static void sigsegv_handler(int sig, siginfo_t *si, void *ptr)
{
    ucontext_t *uc = (ucontext_t *)ptr;

    //printf("SIGSEGV for Address: 0x%p",si->si_addr);
    //printf(" for instruction:0x%lx\n",(long) uc->uc_mcontext.gregs[REG_RIP]);

    /* %rax register maintains a copy of the tainted address at the time the signal was raised.
     * Check to see if the signal was raised because of a tainted pointer.
     * In affirmative, we need to jump to a trampoline similar to Paul's code.TODO
     */
    if(((long) uc->uc_mcontext.gregs[REG_RAX] & (long) (dw_TAG << 48)) != 0) {
      printf("The tainted pointer triggered SIGSEGV : 0x%lx\n", (long) uc->uc_mcontext.gregs[REG_RAX]);
    }

    // If the following exit(0) isn't called, the handler function runs in an
    // infnite loop. TODO
    exit(0);
}


extern void
dw_init(void)
{
  old_malloc_hook = __malloc_hook;
  old_free_hook = __free_hook;

  __malloc_hook = dw_malloc_hook;
  __free_hook = dw_free_hook;

  // TAG 
  dw_MASK = ~(65535ULL << 48);
  dw_TAG = 12345;

  count = 1;
  all_count = 0;
  free_count = 0;
   
  struct sigaction sa;

  sa.sa_flags = SA_SIGINFO;
  sigemptyset(&sa.sa_mask);
  sa.sa_sigaction = sigsegv_handler;
  sigaction(SIGSEGV, &sa, NULL);
}

static void *
dw_malloc_hook(size_t size, const void *caller)
{
  void *result;

  /* Restore all old hooks */
  __malloc_hook = old_malloc_hook;
  __free_hook = old_free_hook;

  /* Call recursively */
  result = malloc (size);
  void *p=result;

  printf("Orig address : %p\n", result);
  result = (void *)(((uintptr_t)result & dw_MASK) | ((uintptr_t)dw_TAG << 48));
  printf("Tagged address : %p\n", result);

  unsigned long return_addr = (unsigned long)__builtin_return_address(0);

  /* Save underlying hooks */
  old_malloc_hook = __malloc_hook;
  old_free_hook = __free_hook;

  //printf("malloc(%zu) called from %p returns %p for %lx \n",
  //      size, caller, result, return_addr);

  /* Restore our own hooks */
  __malloc_hook = dw_malloc_hook;
  __free_hook = dw_free_hook;

  return result;
}

static void
dw_free_hook (void *ptr, const void *caller)
{
  /* Restore all old hooks */
  __malloc_hook = old_malloc_hook;
  __free_hook = old_free_hook;

//  printf("Free: before untaint : %p \n", ptr);
	ptr = (void *)(((intptr_t) ptr << 16) >> 16);
//  printf("Free: after untaint : %p \n", ptr);
 
  /* Call recursively */
  free (ptr);

  /* Save underlying hooks */
  old_malloc_hook = __malloc_hook;
  old_free_hook = __free_hook;

  /* Restore our own hooks */
  __malloc_hook = dw_malloc_hook;
  __free_hook = dw_free_hook;
}
