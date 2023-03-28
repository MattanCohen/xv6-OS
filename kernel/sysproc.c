#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"


uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  char new_msg[32];
  argstr(1, new_msg , MAX_MSG_SIZE);
  exit(n, new_msg);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}
uint64
sys_wait(void)
{
  uint64 exitStatus;
  uint64 exitMsg; 
  argaddr(0, &exitStatus);
  argaddr(1, &exitMsg);
  return wait(exitStatus, exitMsg);
}


uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// return cloom
uint64
sys_memsize(void)
{
  
  // return (uint64)7;
  return myproc()->sz;
}

uint64
sys_set_ps_priority(void)
{
  int new_ps;
  argint(0, &new_ps);
  set_ps_priority(new_ps);

  return 0;
}
uint64
sys_set_cfs_priority(void)
{
  int new_p;
  argint(0, &new_p);
  
  return set_cfs_priority(new_p);
}

uint64
sys_get_cfs_priority(void)
{
  int new_p;
  argint(0, &new_p);
  
  uint64 _cfs_priority;
  argaddr(1, &_cfs_priority);
  uint64 _rtime;
  argaddr(2, &_rtime);
  uint64 _stime;
  argaddr(3, &_stime);
  uint64 _retime;
  argaddr(4, &_retime);
  
  
//   uint64 exitStatus;
//   uint64 exitMsg; 
//   argaddr(0, &exitStatus);
//   argaddr(1, &exitMsg);
//   return wait(exitStatus, exitMsg);
// }
  get_cfs_priority(new_p, _cfs_priority, _rtime, _stime, _retime);
  return 0;
}


uint64
sys_set_policy(void)
{
  int new_policy;
  argint(0, &new_policy);

  return set_policy(new_policy);
}