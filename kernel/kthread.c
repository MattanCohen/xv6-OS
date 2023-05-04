#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

char* ktStateToString(enum kthreadstate s){
return  s == RUNNABLE   ? "Runnable" : 
        s == RUNNING    ? "Running" :  
        s == SLEEPING   ? "Sleeping" :  
        s == USED       ? "USED" :  
        s == UNUSED     ? "UNUSED" :  
        s == ZOMBIE     ? "ZOMBIE" :  
                        "ERROR";
}

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
  // printdebug("freekthread(struct kthread *kt)\n");

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
  printdebug("kthread_create(). calling thread ktid: %d state: %s\n", kthread_id(), ktStateToString(mykthread()->state));

  struct kthread* kt;
  int ktid = -1;

  if ((kt = allockthread(myproc())) == 0){
    printdebug("in ktread creat - alloctread faild\n");
    return -1;
  }

  ktid = kt->ktid;
  kt->trapframe->epc = (uint64)start_func;
  kt->trapframe->sp = (uint64)(stack + stack_size);

  kt->state = RUNNABLE;
  release(&kt->lock);

  printdebug("finished create on ktid %d\n", ktid);

  return ktid; 
}

int kthread_id(){
  printdebug("kthread_id()\n");
  
  int k = -1;
  struct kthread* kt = mykthread();
  struct spinlock* lock = &kt->lock;
  
  acquire(lock);
  k = kt->ktid;
  release(lock);
  
  return k;
  }

int kthread_kill(int ktid){
  printdebug("kthread_kill()\n");
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
void kthread_wakeup(void *chan)
{
  // printdebug("wakeup(void *chan)\n");

  struct proc *p = myproc();

  for (struct kthread* kt = p->kthread; kt < &p->kthread[NKT]; kt++)
  {
    acquire(&kt->lock);
    
    if(kt->state == SLEEPING && kt->chan == chan) {
      kt->state = RUNNABLE;
    }
    release(&kt->lock);
  }

}


void kthread_exit(int status){
  printdebug("kthread_exit()\n");
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
  kthread_wakeup(kt);

  acquire(&kt->lock);
  kt->state = ZOMBIE;
  kt->xstate = status;
  sched();
  panic("panic : zombie kthread exit\n");

}

// makes mykthread sleep on channel join_to, to wait for it to finish
void kthread_sleep(struct kthread* mykthread,struct kthread* join_to){
  printdebug("kthread_sleep()\n");
  
  acquire(&mykthread->lock);


  mykthread->chan = join_to;
  printdebug("kthread sleep before sched() ktid %d\n", mykthread->ktid);
  printdebug("kthread jointo id %d state = %s\n",join_to->ktid , ktStateToString(join_to->state));
  printdebug("kthread mykthread id %d state = %s\n",mykthread->ktid , ktStateToString(mykthread->state));                                           
  mykthread->state = SLEEPING;
  release(&join_to->lock);
  sched();
  printdebug("kthread sleep after sched()\n");
  
  mykthread->chan = 0;

  release(&mykthread->lock);  
  acquire(&join_to->lock);
}

int kthread_join(int ktid, int *status){
  printdebug("kthread_join().\n");
  // struct kthread *mythread = mykthread();
  struct kthread *kt;
  struct kthread *join_to;
  struct proc *p = myproc();
  int found = 0;

  if (kthread_id() == ktid){
    printdebug("to join ktid is my ktid \n");
    return -1;
  }

  // find the index of the thread to join to 
  for (kt = p->kthread ; kt < &p->kthread[NKT] && !found; kt++){
    int holdingkth = 1;
    if(!holding(&kt->lock)){
      acquire(&kt->lock);
      holdingkth = 0;
    }
    if(kt->ktid == ktid){
      join_to = kt;
      found = 1;
      printdebug("kthread_join(). calling thread ktid: %d state - %s, jointo ktid - %d state - %s\n", kthread_id(), ktStateToString(mykthread()->state), ktid, ktStateToString(kt->state));
    }
    else if (!holdingkth)
      release(&kt->lock);
  }

  // if not found retuen error
  if(!found){
    printdebug("thread to join not found \n");
    return -1;
  }




  for(;;){
    
    if(join_to->state == ZOMBIE){
      // if xstate is null return error, if copyout failed return error 
      if(!join_to->xstate || copyout(p->pagetable, (uint64)status,(char *)&join_to->xstate, sizeof(join_to->xstate)) < 0){
        release(&join_to->lock);
        return -1;
      }

      freekthread(join_to);
      release(&join_to->lock);
      return 0;
    }
  
    printdebug("kthread_join going to sleep\n");
    
    kthread_sleep(mykthread(), join_to);  //DOC: wait-sleep
    
  }


}

// general lock to sleep on 