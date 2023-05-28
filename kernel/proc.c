#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

int debug = 0;
char* allowed_swap_algos[4] = {"SCFIFO", "NFUA", "LAPA", "NONE"};
int swap_algo_is_allowed = 0;

struct cpu cpus[NCPU];

struct proc proc[NPROC];

struct proc *initproc;

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
    char *pa = kalloc();
    if(pa == 0)
      panic("kalloc");
    uint64 va = KSTACK((int) (p - proc));
    kvmmap(kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
  }
}

void
initswap(struct proc* p){
  
    // init user pages      
    p->userPages.name = "user pages";
    p->userPages.size = 0;
    p->userPages.maxSize = MAX_PYSC_PAGES;
    for (int i = 0; i < p->userPages.maxSize; i++)
    {
      p->userPages.pages[i] = 0;
    }

    // init swapped pages      
    p->swappedPages.name = "swapped pages";
    p->swappedPages.size = 0;
    p->swappedPages.maxSize = MAX_TOTAL_PAGES - MAX_PYSC_PAGES;
    for (int i = 0; i < p->swappedPages.maxSize; i++)
    {
      p->swappedPages.pages[i] = 0;
    }

    for (int i = 0; i < MAX_TOTAL_PAGES; i++)
    {
      p->swappedPages.pagesCounters[i] = 0;
      p->swappedPages.pagesTriesCounters[i] = 0;
      
      p->userPages.pagesCounters[i] = 0;
      p->userPages.pagesTriesCounters[i] = 0;
    }
    

    // if exists delete p swap file
    int isHolding = holding(&p->lock);
    if (isHolding) release(&p->lock);
    removeSwapFile(p);
    if (isHolding) acquire(&p->lock);
    p->swapFile = 0;
}

// initialize the proc table.
void
procinit(void)
{
  struct proc *p;
  for (int n = 0; n < 4; n++)
  {
    char* i = allowed_swap_algos[n];
    if (!SwapAlgoIsInit(i)) continue;

    swap_algo_is_allowed = 1;
    break;
  }

  printf("Swapping algorithm: " );
  if (swap_algo_is_allowed) printf("%s\n", SWAP_ALGO);
  else printf("%s, but isn't allowed so changing to: %s\n", SWAP_ALGO, allowed_swap_algos[0]);



  initlock(&pid_lock, "nextpid");
  initlock(&wait_lock, "wait_lock");
  for(p = proc; p < &proc[NPROC]; p++) {
      initlock(&p->lock, "proc");
      p->state = UNUSED;
      p->kstack = KSTACK((int) (p - proc));
      

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
  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}

// Return the current struct proc *, or zero if none.
struct proc*
myproc(void)
{
  push_off();
  struct cpu *c = mycpu();
  struct proc *p = c->proc;
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
    if(p->state == UNUSED) {
      goto found;
    } else {
      release(&p->lock);
    }
  }
  return 0;

found:
  p->pid = allocpid();
  p->state = USED;

  // Allocate a trapframe page.
  if((p->trapframe = (struct trapframe *)kalloc()) == 0){
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

  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&p->context, 0, sizeof(p->context));
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;

  initswap(p);
  return p;
}

// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void
freeproc(struct proc *p)
{
  printdebug(debug, "freeproc\n");
  if(p->trapframe)
    kfree((void*)p->trapframe);
  p->trapframe = 0;
  if(p->pagetable)
    proc_freepagetable(p->pagetable, p->sz);
  p->pagetable = 0;
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = UNUSED;
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
  // to/from user space, so not PTE_A.
  if(mappages(pagetable, TRAMPOLINE, PGSIZE,
              (uint64)trampoline, PTE_R | PTE_X) < 0){
    uvmfree(pagetable, 0);
    return 0;
  }

  // map the trapframe page just below the trampoline page, for
  // trampoline.S.
  if(mappages(pagetable, TRAPFRAME, PGSIZE,
              (uint64)(p->trapframe), PTE_R | PTE_W) < 0){
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
  uvmunmap(pagetable, TRAPFRAME, 1, 0);
  uvmfree(pagetable, sz);
  initswap(myproc());
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
  p->trapframe->epc = 0;      // user program counter
  p->trapframe->sp = PGSIZE;  // user stack pointer

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;

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

// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *p = myproc();
  printdebug(debug, "proc.c::%s()\n",__func__ );
  // PrintPageData(&myproc()->userPages);
  // PrintPageData(&myproc()->swappedPages);

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy user memory from parent to child.
  if(uvmcopy(np, p->pagetable, np->pagetable, p->sz) < 0){
    freeproc(np);
    release(&np->lock);
    return -1;
  }
  np->sz = p->sz;

  // copy saved user registers.
  *(np->trapframe) = *(p->trapframe);

  // Cause fork to return 0 in the child.
  np->trapframe->a0 = 0;

  // increment reference counts on open file descriptors.
  for(i = 0; i < NOFILE; i++)
    if(p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  np->cwd = idup(p->cwd);

  safestrcpy(np->name, p->name, sizeof(p->name));

  pid = np->pid;

  release(&np->lock);

  acquire(&wait_lock);
  np->parent = p;
  release(&wait_lock);

  acquire(&np->lock);
  np->state = RUNNABLE;
  release(&np->lock);

  if (!debug){
    return pid;
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
  p->state = ZOMBIE;

  release(&wait_lock);

  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(uint64 addr)
{
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
        if(pp->state == ZOMBIE){
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
    if(!havekids || killed(p)){
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
  struct proc *p;
  struct cpu *c = mycpu();
  
  c->proc = 0;
  for(;;){
    // Avoid deadlock by ensuring that devices can interrupt.
    intr_on();

    for(p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if(p->state == RUNNABLE) {
        // Switch to chosen process.  It is the process's job
        // to release its lock and then reacquire it
        // before jumping back to us.
        AppendCounters(p);
        p->state = RUNNING;
        c->proc = p;
        swtch(&c->context, &p->context);

        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;
      }
      release(&p->lock);
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
  struct proc *p = myproc();

  if(!holding(&p->lock))
    panic("sched p->lock");
  if(mycpu()->noff != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(intr_get())
    panic("sched interruptible");

  intena = mycpu()->intena;
  swtch(&p->context, &mycpu()->context);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  struct proc *p = myproc();
  acquire(&p->lock);
  p->state = RUNNABLE;
  sched();
  release(&p->lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void
forkret(void)
{
  static int first = 1;

  // Still holding p->lock from scheduler.
  release(&myproc()->lock);

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
  struct proc *p = myproc();
  
  // Must acquire p->lock in order to
  // change p->state and then call sched.
  // Once we hold p->lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup locks p->lock),
  // so it's okay to release lk.

  acquire(&p->lock);  //DOC: sleeplock1
  release(lk);

  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  release(&p->lock);
  acquire(lk);
}

// Wake up all processes sleeping on chan.
// Must be called without any p->lock.
void
wakeup(void *chan)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    if(p != myproc()){
      acquire(&p->lock);
      if(p->state == SLEEPING && p->chan == chan) {
        p->state = RUNNABLE;
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
      if(p->state == SLEEPING){
        // Wake process from sleep().
        p->state = RUNNABLE;
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
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    printf("%d %s %s", p->pid, state, p->name);
    printf("\n");
  }
}


//------------------PAGE META DATA------------------

void PrintPagingError(struct proc* p, char* msg){
  printf("!!!!!!!!!!! PAGING ERROR - ", p->pid);
  printf(msg);
  printf("\n");
  if (debug){
    printf("\nprocess %d data pages:\n", p->pid);
    PrintPageData(&p->userPages);
    PrintPageData(&p->swappedPages);
  }
  exit(-9);
}

void PrintPageData(pagedata* data){
  printf("printing page data \"%s\": \n", data->name);
  for (int i = 0; i < data->maxSize; i++)
  {
    printf("\tpage %d : %p counter = %d\n", i, data->pages[i], data->pagesCounters[i]);
  }
}

int RemoveFromPageData(pagedata* data, pte_t page){
  for (int i = 0; i < data->maxSize; i++)
  {
    if (data->pages[i] && data->pages[i] == page){
      data->pages[i] = 0;
      data->pagesCounters[i] = 0;
      data->pagesTriesCounters[i] = 0;
      data->size--;
      return 1;
    }
  }
  return 0;
}

int AddToPageData(pagedata* data, pte_t page){
  for (int i = 0; i < data->maxSize; i++)
  {
    if (!data->pages[i]){
      data->pages[i] = page;
      data->pagesCounters[i] = 0;
      data->pagesTriesCounters[i] = 0;
      data->size++;
      return 1;
    }
  }
  return 0;
}

void AddPage(struct proc* p,pte_t page){
  pagedata* userPages = &p->userPages;
  pagedata* swappedPages = &p->swappedPages;
  int failed = 0;
  int at = -1;
  if (userPages->size < userPages->maxSize){
    at = userPages->size;
    failed = AddToPageData(userPages, page) || failed;
  }
  else if (swappedPages->size < swappedPages->maxSize){
    at = userPages->size;
    if (swappedPages->size == 0){
      printdebug(debug, "creating swapfile\n");
      createSwapFile(p);
    }
    failed = AddToPageData(swappedPages, page) || failed;
    if(!failed) writeToSwapFile(p, (char*)page, p->swapFile, PGSIZE);
  }
  else{
    PrintPagingError(p, "trying to add a page when more than the allowed number exists!");
  }
  printdebug(debug,"pid %d- proc.c::AddPage(%p) adding page #%d %s\n",p->pid, page, at ,failed ? "succeeded" : "failed");
  if (!failed){
    PrintPageData(&p->userPages);
    PrintPageData(&p->swappedPages);
  }
}

void RemovePage(struct proc* p, pte_t page){
  pagedata* userPages = &p->userPages;
  pagedata* swappedPages = &p->swappedPages;
  int failed = 0;
  int at = -1;

  if (!page) PrintPagingError(p, "RemovePage recieved empty page");

  if (swappedPages->size >= 1){
    failed = RemoveFromPageData(swappedPages, page) || failed;
    at = swappedPages->size;
    if(!failed) readFromSwapFile(p, (char*)page, p->swapFile, PGSIZE);
    if (swappedPages->size == 0){
      printdebug(debug, "removing swapfile\n");
      removeSwapFile(p);
    }
  }
  else if (userPages->size == 0){
    PrintPagingError(p, "trying to remove page but no pages allocated");
  }
  else{
    failed = RemoveFromPageData(userPages, page) || failed;
    at = userPages->size;
  }
  printdebug(debug,"pid %d- proc.c::RemovePage(%p) removing page #%d %s\n",p->pid, page, at,  failed ? "succeeded" : "failed");
}



pte_t* GetPageToRemove(){
  pagedata* userPages = &myproc()->userPages;
  pagedata* swappedPages = &myproc()->swappedPages;
  printdebug(debug,"getting page to remove\n");

  for (int i = swappedPages->maxSize - 1; i >= 0; i--)
  {
    if (swappedPages->pages[i] != 0) return &swappedPages->pages[i];
  }
  
  for (int i = userPages->maxSize - 1; i >= 0; i--)
  {
    if (userPages->pages[i] != 0) return &userPages->pages[i];
  }
  PrintPagingError(myproc(), "proc.c::GetPageToRemove(void) Cant get page to remove");
  return 0;
}

int IsPageInPageData(pte_t page, pagedata* data){
  for (int i = 0; i < data->size; i++)
  {
    if (page == data->pages[i]){
      printdebug(debug, "page %p is in page data %s\n", page, data->name);
      return 1;
    }
  }
  
  return 0;
}


void AppendCounters(struct proc* p){
  for (int i = 0; i < p->swappedPages.maxSize; i++)
  {
    if (p->swappedPages.pages[i] & PTE_A){
      // p->swappedPages.pagesCounters[i]++;
      p->swappedPages.pagesCounters[i] >>= 1;  // Shift right by one bit
      p->swappedPages.pagesCounters[i] |= (1 << (sizeof(int) * 8 - 1));  // Set most significant bit to 1

      p->swappedPages.pages[i] = p->swappedPages.pages[i] - PTE_A;
    }
  }
  for (int i = 0; i < p->userPages.maxSize; i++)
  {
    if (p->userPages.pages[i] & PTE_A){
      // p->userPages.pagesCounters[i]++;
      p->swappedPages.pagesCounters[i] >>= 1;  // Shift right by one bit
      p->swappedPages.pagesCounters[i] |= (1 << (sizeof(int) * 8 - 1));  // Set most significant bit to 1
      p->userPages.pages[i] = p->userPages.pages[i] - PTE_A;
    }
  }
}

//    not frequently used + aging
pte_t GetPageNFUA(struct proc* p, pte_t page){
  int minCounter = 2147000000;
  pte_t pageToMove = (pte_t)0;
  for (int i = 0; i < p->userPages.maxSize; i++)
    if (p->userPages.pagesCounters[i] < minCounter){
      pageToMove = p->userPages.pages[i];
      minCounter = p->userPages.pagesCounters[i];
    }
  return pageToMove;
}
//    least accessed page + aging
pte_t GetPageLAPA(struct proc* p, pte_t page){
  int minOnes = __INT_MAX__;
  pte_t pageToMove = (pte_t)0;
  int minCounter = __INT_MAX__;  
  for (int i = 0; i < p->userPages.maxSize; i++) {
    int counter = p->userPages.pagesCounters[i];
    int ones = 0;
    
    // Count the number of ones in the counter using a bit manipulation approach
    while (counter != 0) {
      ones += counter & 1;
      counter >>= 1;
    }
    
    if (ones < minOnes) {
      pageToMove = p->userPages.pages[i];
      minOnes = ones;
      minCounter = counter;
    }
    
    if(ones == minOnes){
      if(counter < minCounter){
        pageToMove = p->userPages.pages[i];
        minOnes = ones;
        minCounter = counter;
      }
    }
  }
  
  return pageToMove;
}

//  second chance FIFO
pte_t GetPageSCFIFO(struct proc* p, pte_t page){
  int minCounter = 2147000000;
  pte_t pageToMove = (pte_t)0;
  for (int i = 0; i < p->userPages.maxSize; i++)
    if (p->userPages.pagesCounters[i] < minCounter){
      // second chance
      if (p->userPages.pagesTriesCounters[i] == 0 && p->userPages.pagesCounters[i] & PTE_A){
        p->userPages.pagesCounters[i] = p->userPages.pagesCounters[i] - PTE_A;
        p->userPages.pagesTriesCounters[i]++;
        continue;
      }
      pageToMove = p->userPages.pages[i];
      minCounter = p->userPages.pagesCounters[i];
    }
  return pageToMove;
}
//    no paging
pte_t GetPageNONE(struct proc* p, pte_t page){
  return page;
}

int SwapAlgoIsInit(char* s){return strncmp((char*)SWAP_ALGO, s, sizeof(s)) == 0;}

int SwapAlgoIs(char* s){
  if (!swap_algo_is_allowed) return strncmp(allowed_swap_algos[0], s, sizeof(s)) == 0;
  return strncmp((char*)SWAP_ALGO, s, sizeof(s)) == 0;
  }

void ReplacePage(struct proc* p, pte_t page){
  pte_t pageToMove = (pte_t)0;
  if (SwapAlgoIs("NFUA")) pageToMove = GetPageNFUA(p, page);
  else if (SwapAlgoIs("LAPA")) pageToMove = GetPageLAPA(p, page);
  else if (SwapAlgoIs("SCFIFO")) pageToMove = GetPageSCFIFO(p, page);
  else if (SwapAlgoIs("NONE")) return;
  
  
  RemoveFromPageData(&p->swappedPages, page);
  RemoveFromPageData(&p->userPages, pageToMove);
  AddToPageData(&p->userPages, page);
  AddToPageData(&p->swappedPages, pageToMove);
}
//------------------PAGE META DATA------------------
