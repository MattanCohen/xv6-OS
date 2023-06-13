
#include <stdarg.h>

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

#define C(x)  ((x)-'@')  // Control-x

struct {
  struct spinlock lock;
  uint8 seed;
} random;

uint8 lfsr_char(uint8 lfsr)
{
  uint8 bit = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 4)) & 0x01;
  // random.seed = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 4)) & 0x01;
  lfsr = (lfsr >> 1) | (bit << 7);
  // lfsr = (lfsr >> 1) | (random.seed << 7);
  return lfsr;
}

int isok(int n, uint64 src){return n != 1 && either_copyin(&random.seed, src, src, n) != -1;}
// write (int, uint64, int)
int randomwrite(int fd, uint64 src, int n){ return isok(n,src) ? 1 : -1;}

// read (int, uint64, int)
int randomread(int fd, uint64 dst, int n){
  int writtenBytes = 0;
  char lfsr = random.seed;


  acquire(&random.lock);

  while(n > 0){

    lfsr = lfsr_char(lfsr);
    printf("random write : %d\n", lfsr);
    
    // copy the input byte to the user-space buffer.
    if(either_copyout(fd, dst, &lfsr, 1) == -1)
      break;

    dst++;
    --n;
    writtenBytes++;
  }
  release(&random.lock);

  return writtenBytes;
}

void randominit(void)
{
  initlock(&random.lock, "random");

  random.seed = 0x2A;

  devsw[RANDOM].read = randomread;
  devsw[RANDOM].write = randomwrite;
}
