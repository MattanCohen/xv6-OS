#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

extern struct proc proc[NPROC];

// initalize kthread table
void kthreadinit(struct proc *p)
{
  // printdebug("kthreadinit(struct proc *p)\n");
  initlock(&p->ktidlock, "ktidlock");
 
  for (struct kthread *kt = p->kthread; kt < &p->kthread[NKT]; kt++)
  {
    initlock(&kt->lock, "ktlock");
    kt->state = UNUSED;
    kt->pcb = p;

    // WARNING: Don't change this line!
    // get the pointer to the kernel stack of the kthread
    kt->kstack = KSTACK((int)((p - proc) * NKT + (kt - p->kthread)));
  }
}

struct kthread *mykthread()
{
  // printdebug("mykthread()\n");
  // TODO check for push and pop 
  // push_off();
  struct cpu *c = mycpu();
  struct kthread *kt = c->kthread;
  // pop_off();
  return kt;
}

struct trapframe *get_kthread_trapframe(struct proc *p, struct kthread *kt)
{
  return p->base_trapframes + ((int)(kt - p->kthread));
}

// TODO: delte this after you are done with task 2.2
void allocproc_help_function(struct proc *p) {
}

int allocktid(struct proc* p){
  // printdebug("allocktid(struct proc* p)\n");

    int newktid;
    acquire(&p->ktidlock);
    newktid = p->nextktid ;
    p->nextktid = p->nextktid + 1;
    release(&p->ktidlock);
    return newktid;
}

struct kthread* allockthread(struct proc* p){
  printdebug("allockthread(struct proc* p)\n");
  for (struct kthread *kt = p->kthread; kt < &p->kthread[NKT]; kt++)
  {
    acquire(&kt->lock);
    if (kt->state != UNUSED){
      release(&kt->lock);
      continue;
    }

    kt->ktid = allocktid(p);
    kt->state = USED;
    kt->trapframe = get_kthread_trapframe(p, p->kthread);
    memset(&kt->context, 0, sizeof(kt->context));
    kt->context.ra = (uint64)forkret;
    printdebug("context.ra %d\n", (int)kt->context.ra);
    kt->context.sp = kt->kstack + PGSIZE;
    return kt;
  }
  return 0;
}

void freekthread(struct kthread *kt){
  printdebug("freekthread(struct kthread *kt)\n");

  kt->chan = 0;
  kt->killed = 0;
  kt->xstate = 0;
  kt->ktid = 0;
  kt->pcb = 0;
  kt->trapframe = 0;
  kt->state = UNUSED;
  return;
}
