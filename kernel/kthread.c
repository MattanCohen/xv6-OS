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
  push_off();
  struct cpu *c = mycpu();
  struct kthread *kt = c->kthread;
  pop_off();
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

    kt->killed = 0;
    kt->ktid = allocktid(p);
    kt->state = USED;
    kt->trapframe = get_kthread_trapframe(p, kt);
    kt->pcb = p;
    
    memset(&kt->context, 0, sizeof(kt->context));
    kt->context.ra = (uint64)forkret;
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
  memset(&kt->context, 0, sizeof(kt->context));
  return;
}


int kthread_create( void *(*start_func)(), void *stack, uint stack_size ){
  
  struct kthread* kt;
  if ((kt = allockthread(myproc())) == 0){
    return -1;
  }
  if (sizeof(stack) != stack_size){
    return -1;
  }

  kt->state = RUNNABLE;
  kt->kstack = (uint64)stack;
  kt->trapframe->epc = (uint64)start_func;
  kt->trapframe->sp = (uint64)(stack + stack_size);
  return 0; 
}

int kthread_id(){
  int k = -1;
  struct kthread* kt = mykthread();
  struct spinlock* lock = &kt->lock;
  
  acquire(lock);
  k = kt->ktid;
  release(lock);
  
  return k;
  }

int kthread_kill(int ktid){
  struct proc* p = myproc();
  struct kthread* kt;

  for (kt = p->kthread; kt < &p->kthread[NKT]; kt++)
  {
    acquire(&kt->lock);

    if(kt->ktid == ktid){
      kt->killed = 1;

      if(kt->state == SLEEPING){
        kt->state = RUNNABLE;
      }
      release(&kt->lock);
      return 0;
    }

    release(&kt->lock);
  }

  return -1;

}


void kthread_exit(int status){
  // struct proc* p = myproc();
  struct kthread* kt = mykthread();


  int everyone_are_dead = 1;

  for (struct kthread* i = kt->pcb->kthread; i < &kt->pcb->kthread[NKT] && everyone_are_dead; i++)
  {
    acquire(&i->lock);
    if (i->killed == 0)
      everyone_are_dead = 0;
    release(&i->lock);
  }

  if (everyone_are_dead){
    exit(status);
  }
  
  // if anyone waiting for me to die
  wakeup(kt);

  acquire(&kt->lock);
  kt->state = ZOMBIE;
  kt->xstate = status;
  sched();
  panic("panic : zombie kthread exit\n");

}

// makes src sleep on channel dst, to wait for it to finish
void kthread_sleep(struct kthread* src,struct kthread* dst){
  
  acquire(&src->lock);

  src->chan = dst;
  src->state = SLEEPING;

  sched();
 
  src->chan = 0;

  release(&src->lock);  
}

int kthread_join(int ktid, int *status){
  // struct kthread *mythread = mykthread();
  struct kthread *kt;
  struct proc *p = myproc();
  int index = -1;

  // find the index of the thread to join to 
  for (int i =0 ; i < NKT; i++){
    kt = &p->kthread[i];
    acquire(&kt->lock);
    if(kt->ktid == ktid)
      index = i;
    release(&kt->lock);
  }

  // if not found retuen error
  if(index == -1){
    return -1;
  }

  kt = &p->kthread[index];

  for(;;){
    acquire(&kt->lock);

    if (kt->ktid == ktid){
      return -1;
    }
    if(kt->state == ZOMBIE){
      // if xstate is null return error 
      if(kt->xstate){
        // if copyout failed return error 
        if(copyout(p->pagetable, (uint64)status,(char *)&kt->xstate, sizeof(kt->xstate)) < 0){
          release(&kt->lock);
          return -1;
        }
      }

      freekthread(kt);
      release(&kt->lock);
      return 0;
    }

    kthread_sleep(mykthread(), kt);  //DOC: wait-sleep
    
  }


}

// general lock to sleep on 