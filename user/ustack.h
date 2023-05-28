#include "kernel/types.h"


typedef long Align;

union header {
  struct {
    union header *ptr;
    uint size;
  } s;
  Align x;
};

typedef union header Toy;


void * ustack_malloc(uint len);
int ustack_free(void);