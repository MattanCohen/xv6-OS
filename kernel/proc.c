#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

char* StateToString(enum kthreadstate s){
return  s == RUNNABLE   ? "Runnable" : 
        s == RUNNING    ? "Running" :  
        s == SLEEPING   ? "Sleeping" :  
        s == USED       ? "USED" :  
        s == UNUSED     ? "UNUSED" :  
        s == ZOMBIE     ? "ZOMBIE" :  
                        "ERROR";
}

struct cpu cpus[NCPU];

struct proc proc[NPROC];

struct proc *initproc;

struct kthread *initkthread;

int nextpid = 1;
struct spinlock pid_lock;

extern void forkret(void);

static void freeproc(struct proc *p);

extern char trampoline[]; // trampoline.S

// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;



// Allocate a page for each process's kernel stack.
// Map it high in memory, followed by an invalid
// guard page.
void
proc_mapstacks(pagetable_t kpgtbl)
{
  struct proc *p;
  
  for(p = proc; p < &proc[NPROC]; p++) {
    for (struct kthread *kt = p->kthread; kt < &p->kthread[NKT]; kt++) {
      char *pa = kalloc();
      if(pa == 0)
        panic("kalloc");
      uint64 va = KSTACK((int) ((p - proc) * NKT + (kt - p->kthread)));
      kvmmap(kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
    }
  }
}

// initialize the proc table.
void
procinit(void)
{
  struct proc *p;
  
  initlock(&pid_lock, "nextpid");
  initlock(&wait_lock, "wait_lock");
  for(p = proc; p < &proc[NPROC]; p++) {
      initlock(&p->lock, "proc");
      p->state = PUNUSED;
      kthreadinit(p);
  }
}

// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
int
cpuid()
{
  int id = r_tp();
  return id;
}

// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu*
mycpu(void)
{
  // printdebug("mycpu(void)\n");
  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}

// Return the current struct proc *, or zero if none.
struct proc*
myproc(void)
{
  // printdebug("myproc(void)\n");
  push_off();
  struct cpu *c = mycpu();
  
  // struct proc *p = c->kthread->pcb;
  struct proc *p = 0;
  if(c->kthread){
    p = c->kthread->pcb;
  }
  
  pop_off();
  return p;
}

int
allocpid()
{

  int pid;
  
  acquire(&pid_lock);
  pid = nextpid;
  nextpid = nextpid + 1;
  release(&pid_lock);

  return pid;
}

// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, or a memory allocation fails, return 0.
static struct proc*
allocproc(void)
{

  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state == PUNUSED) {
      goto found;
    } else {
      release(&p->lock);
    }
  }
  return 0;

found:
  p->pid = allocpid();
  p->state = PUSED;

  // Allocate a trapframe page.
  if((p->base_trapframes = (struct trapframe *)kalloc()) == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // An empty user page table.
  p->pagetable = proc_pagetable(p);
  if(p->pagetable == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // acquire(&p->ktidlock);
  p->nextktid = 1;
  // release(&p->ktidlock);

  allockthread(p);
  
  return p;
}

// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void
freeproc(struct proc *p)
{
  if(p->base_trapframes)
    kfree((void*)p->base_trapframes);
  p->base_trapframes = 0;
  if(p->pagetable)
    proc_freepagetable(p->pagetable, p->sz);
  p->pagetable = 0;
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  // moved from PCB to KTCB
  // p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = PUNUSED;
  for (struct kthread *kt = p->kthread; kt < &p->kthread[NKT]; kt++)
  {
    freekthread(kt);
  }
}

// Create a user page table for a given process, with no user memory,
// but with trampoline and trapframe pages.
pagetable_t
proc_pagetable(struct proc *p)
{
  pagetable_t pagetable;

  // An empty page table.
  pagetable = uvmcreate();
  if(pagetable == 0)
    return 0;

  // map the trampoline code (for system call return)
  // at the highest user virtual address.
  // only the supervisor uses it, on the way
  // to/from user space, so not PTE_U.
  if(mappages(pagetable, TRAMPOLINE, PGSIZE,
              (uint64)trampoline, PTE_R | PTE_X) < 0){
    uvmfree(pagetable, 0);
    return 0;
  }

  // map the trapframe page just below the trampoline page, for
  // trampoline.S.
  if(mappages(pagetable, TRAPFRAME(0), PGSIZE,
              (uint64)(p->base_trapframes), PTE_R | PTE_W) < 0){
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmfree(pagetable, 0);
    return 0;
  }

  return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void
proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
  uvmunmap(pagetable, TRAMPOLINE, 1, 0);
  uvmunmap(pagetable, TRAPFRAME(0), 1, 0);
  uvmfree(pagetable, sz);
}

// a user program that calls exec("/init")
// assembled from ../user/initcode.S
// od -t xC ../user/initcode
uchar initcode[] = {
  0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02,
  0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x35, 0x02,
  0x93, 0x08, 0x70, 0x00, 0x73, 0x00, 0x00, 0x00,
  0x93, 0x08, 0x20, 0x00, 0x73, 0x00, 0x00, 0x00,
  0xef, 0xf0, 0x9f, 0xff, 0x2f, 0x69, 0x6e, 0x69,
  0x74, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

// Set up first user process.
void
userinit(void)
{
  struct proc *p;

  p = allocproc();
  initproc = p;
  
  // allocate one user page and copy initcode's instructions
  // and data into it.
  uvmfirst(p->pagetable, initcode, sizeof(initcode));
  p->sz = PGSIZE;

  // prepare for the very first "return" from kernel to user.
  //approved
  p->kthread[0].trapframe->epc = 0;      // user program counter
  //approved
  p->kthread[0].trapframe->sp = PGSIZE;  // user stack pointer
  //approved
  p->kthread[0].state = RUNNABLE;

  //approved
  release(&p->kthread[0].lock);

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  release(&p->lock);
}

// Grow or shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{

  uint64 sz;
  struct proc *p = myproc();

  sz = p->sz;
  if(n > 0){
    if((sz = uvmalloc(p->pagetable, sz, sz + n, PTE_W)) == 0) {
      return -1;
    }
  } else if(n < 0){
    sz = uvmdealloc(p->pagetable, sz, sz + n);
  }
  p->sz = sz;
  return 0;
}

int i = 0;

void counter(int reset){
  if (reset) i = 0;
  printdebug("%d\n", ++i);
}

void copy_kthread_for_fork(struct kthread* out_kt, struct kthread* kt_in){
  
  out_kt->state = kt_in->state;
  out_kt->chan = kt_in->chan;
  out_kt->xstate = kt_in->xstate;
  out_kt->ktid = kt_in->ktid;
  *(out_kt->trapframe) = *(kt_in->trapframe);

}


// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
int
fork(void)
{

  int i, pid;
  struct proc *np;
  // struct kthread *kt = mykthread();
  struct proc *p = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy user memory from parent to child.
  if(uvmcopy(p->pagetable, np->pagetable, p->sz) < 0){
    freeproc(np);
    release(&np->lock);
    return -1;
  }

  np->state = PUSED;
  np->sz = p->sz;
  
  // // copy saved user registers.
  // *(np->base_trapframes) = *(p->base_trapframes);

  
  


  // increment reference counts on open file descriptors.
  for(i = 0; i < NOFILE; i++)
    if(p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  np->cwd = idup(p->cwd);

  safestrcpy(np->name, p->name, sizeof(p->name));

  pid = np->pid;

  //approved
  release(&np->kthread[0].lock);
  release(&np->lock);

  acquire(&wait_lock);
  np->parent = p;
  release(&wait_lock);


  // copy threads from one another
  acquire(&np->lock);
  struct kthread* kt;
  // struct kthread* nkt;
  for (int i =0 ; i < NKT ; i++){
  // for (kt = p->kthread, nkt = np->kthread; kt < &p->kthread[NKT] && nkt < &np->kthread[NKT]; kt++, nkt++)
  // {
    
    acquire(&p->kthread[i].lock);
    if(p->kthread[i].state != UNUSED){
      // the first thread is allocated in allocproc alloc only from the second one
      if (np->kthread[i].state == UNUSED){
        if (&np->kthread[i] == (kt = allockthread(np))) release(&np->kthread[i].lock);
        else {release(&kt->lock); printf("shouldnt get here\n");}
      }
      copy_kthread_for_fork(&np->kthread[i], &p->kthread[i]);
    }
    release(&p->kthread[i].lock);
  }
  release(&np->lock);
  
  for(struct kthread * t = np->kthread ; t < &np->kthread[NKT] ; t++){
    acquire(&t->lock);
    if(t->state == RUNNING){
      t->state = RUNNABLE;
      t->trapframe->a0 = 0;
      release(&t->lock);
      break;
    }
    release(&t->lock);
  }

  return pid;
}

// Pass p's abandoned children to init.
// Caller must hold wait_lock.
void
reparent(struct proc *p)
{
  struct proc *pp;

  for(pp = proc; pp < &proc[NPROC]; pp++){
    if(pp->parent == p){
      pp->parent = initproc;
      wakeup(initproc);
    }
  }
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait().
void
exit(int status)
{
  struct proc *p = myproc();

  if(p == initproc)
    panic("init exiting");

  // Close all open files.
  for(int fd = 0; fd < NOFILE; fd++){
    if(p->ofile[fd]){
      struct file *f = p->ofile[fd];
      fileclose(f);
      p->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(p->cwd);
  end_op();
  p->cwd = 0;

  acquire(&wait_lock);

  // Give any children to init.
  reparent(p);

  // Parent might be sleeping in wait().
  wakeup(p->parent);

  acquire(&p->lock);

  p->xstate = status;
  p->state = PZOMBIE;

  for (struct kthread* kt = p->kthread; kt < &p->kthread[NKT]; kt++)
  {
    acquire(&kt->lock);
    kt->xstate = status;
    kt->state = ZOMBIE;
    // TODO : check if works
    release(&kt->lock);
  }

  release(&p->lock);
  release(&wait_lock);
  
  acquire(&mykthread()->lock);
  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(uint64 addr)
{
  // printdebug("wait(uint64 addr)\n");
  struct proc *pp;
  int havekids, pid;
  struct proc *p = myproc();

  acquire(&wait_lock);

  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(pp = proc; pp < &proc[NPROC]; pp++){
      if(pp->parent == p){
        // make sure the child isn't still in exit() or swtch().
        acquire(&pp->lock);

        havekids = 1;
        if(pp->state == PZOMBIE){
          // Found one.
          pid = pp->pid;
          if(addr != 0 && copyout(p->pagetable, addr, (char *)&pp->xstate,
                                  sizeof(pp->xstate)) < 0) {
            release(&pp->lock);
            release(&wait_lock);
            return -1;
          }
          freeproc(pp);
          release(&pp->lock);
          release(&wait_lock);
          return pid;
        }
        release(&pp->lock);
      }
    }
    
    // No point waiting if we don't have any children.
    if(!havekids || kthreadkilled(mykthread())){
      release(&wait_lock);
      return -1;
    }
    
    // Wait for a child to exit.
    sleep(p, &wait_lock);  //DOC: wait-sleep
  }
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.
void
scheduler(void)
{
  printdebug("in scheduler()\n");
  struct proc *p;
  struct cpu *c = mycpu();
  
  c->kthread = 0;
  for(;;){
    // Avoid deadlock by ensuring that devices can interrupt.
    intr_on();
    for(p = proc; p < &proc[NPROC]; p++) {
      int num_of_running_procs = 0;
      for (struct kthread* kt = p->kthread; kt < &p->kthread[NKT]; kt++)
      {
        acquire(&kt->lock);
        if (kt->state == RUNNING){
          num_of_running_procs++;
        } 
        release(&kt->lock);
      }
      if (num_of_running_procs > 1){
        break;
      }
      int found = 0;
      for (struct kthread* kt = p->kthread; kt < &p->kthread[NKT] && !found; kt++)
      {
        acquire(&kt->lock);

        if(kt->state == RUNNABLE) {
          found = 1;
          if(p->state != PUSED)
              panic("kthread is runnable while proc is not used\n");
          
          // Switch to chosen process.  It is the process's job
          // to release its lock and then reacquire it
          // before jumping back to us.
          kt->state = RUNNING;
          c->kthread = kt;
          // moved from PCB to KTCB
          swtch(&c->context, &kt->context);
          // Process is done running for now.
          // It should have changed its p->state before coming back.
          c->kthread = 0;

          if (kt->state == RUNNING){
            kt--;
          }

        
        }
          
        release(&kt->lock);
      }
    }
  }
}

// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{

  int intena;
  // struct proc *p = myproc();
  struct kthread *kt = mykthread();

  // if(!holding(&p->lock))
  //   panic("sched p->lock");
  if(!holding(&kt->lock))
    panic("sched kt->lock");    
  if(mycpu()->noff != 1)
    panic("sched locks");
  if(kt->state == RUNNING)
    panic("sched running");
  if(intr_get())
    panic("sched interruptible");

  intena = mycpu()->intena;
  // moved from PCB to KTCB

  swtch(&kt->context, &mycpu()->context);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  // printdebug("yield() ");
  // if (mykthread())
  //   printdebug("with mykthread kid %d on state - %s\n", kthread_id(), StateToString(mykthread()->state));
  // else
  //   printdebug("with no mykthread\n");
    
  struct kthread *kt = mykthread();

  acquire(&kt->lock);

  kt->state = RUNNABLE;
  sched();

  release(&kt->lock);

}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void
forkret(void)
{
  static int first = 1;

  // Still holding p->lock from scheduler.
  release(&mykthread()->lock);
  // release(&myproc()->lock);

  if (first) {
    // File system initialization must be run in the context of a
    // regular process (e.g., because it calls sleep), and thus cannot
    // be run from main().
    first = 0;
    fsinit(ROOTDEV);
  }

  usertrapret();
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  // printdebug("sleep chan : %d\n", &chan);
  struct kthread *kt = mykthread();
  
  
  // Must acquire p->lock in order to
  // change p->state and then call sched.
  // Once we hold p->lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup locks p->lock),
  // so it's okay to release lk.

  // acquire(&p->lock);  //DOC: sleeplock1
  acquire(&kt->lock);  //DOC: sleeplock1
  release(lk);

  // moved from PCB to KTCB
  // // Go to sleep.
  // p->chan = chan;
  kt->chan = chan;
  kt->state = SLEEPING;

  sched();
 
  // moved from PCB to KTCB
  kt->chan = 0;

  // Reacquire original lock.
  release(&kt->lock);
  // release(&p->lock);
  acquire(lk);
}

// Wake up all processes sleeping on chan.
// Must be called without any p->lock.
void
wakeup(void *chan)
{
  // printdebug("wakeup(void *chan)\n");

  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    if(p != myproc()){
      acquire(&p->lock);
      for (struct kthread* kt = p->kthread; kt < &p->kthread[NKT]; kt++)
      {
        acquire(&kt->lock);
        
        if(kt->state == SLEEPING && kt->chan == chan) {
          kt->state = RUNNABLE;
        }
        release(&kt->lock);
      }
      release(&p->lock);
    }
  }
}

// Kill the process with the given pid.
// The victim won't exit until it tries to return
// to user space (see usertrap() in trap.c).
int
kill(int pid)
{

  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->pid == pid){
      p->killed = 1;
      
      for (struct kthread* kt = p->kthread; kt < &p->kthread[NKT]; kt++){
        acquire(&kt->lock);
        kt->killed = 1;
        if (kt->state == SLEEPING)
          kt->state = RUNNABLE;        
        release(&kt->lock);
      }
      

      release(&p->lock);
      return 0;
    }
    release(&p->lock);
  }
  return -1;
}

void
setkilled(struct proc *p)
{

  acquire(&p->lock);
  p->killed = 1;
  for (struct kthread* kt = p->kthread; kt < &p->kthread[NKT]; kt++){
    acquire(&kt->lock);
    kt->killed = 1;
    release(&kt->lock);
  }
  release(&p->lock);
}

int
killed(struct proc *p)
{
  int k;
  acquire(&p->lock);
  k = p->killed;
  release(&p->lock);
  return k;
}

/**
 * checking for start_kt[0]....start_kt[NKT]:
 *    if everyone are killed, returns 1
 *    if one is alive,  returns 0
*/
int
kthreadkilled(struct kthread* kt)
{
  int k ;
  acquire(&kt->pcb->lock);
  acquire(&kt->lock);
  k = kt->killed || kt->pcb->killed;
  release(&kt->lock);
  release(&kt->pcb->lock);
  return k;
}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int
either_copyout(int user_dst, uint64 dst, void *src, uint64 len)
{
  struct proc *p = myproc();
  if(user_dst){
    return copyout(p->pagetable, dst, src, len);
  } else {
    memmove((char *)dst, src, len);
    return 0;
  }
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int
either_copyin(void *dst, int user_src, uint64 src, uint64 len)
{
  struct proc *p = myproc();
  if(user_src){
    return copyin(p->pagetable, dst, src, len);
  } else {
    memmove(dst, (char*)src, len);
    return 0;
  }
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{

  static char *states[] = {
  [UNUSED]    "unused",
  [USED]      "used",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  struct proc *p;
  char *state;

  printf("\n");
  for(p = proc; p < &proc[NPROC]; p++){
    if(p->state == PUNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    printf("%d %s %s", p->pid, state, p->name);
    printf("\n");
  }
}
