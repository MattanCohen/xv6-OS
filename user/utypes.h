#include "kernel/types.h"
#include "user/user.h"

#define MAX_UTHREADS  4
#define STACK_SIZE  4000

typedef int bool;

// Saved registers for context switches.
struct context {
    uint64 ra;
    uint64 sp;

    // callee saved
    uint64 s0;
    uint64 s1;
    uint64 s2;
    uint64 s3;
    uint64 s4;
    uint64 s5;
    uint64 s6;
    uint64 s7;
    uint64 s8;
    uint64 s9;
    uint64 s10;
    uint64 s11;
};
typedef struct context tcontext;

enum sched_priority { LOW, MEDIUM, HIGH };
typedef enum sched_priority sched_priority;

/* Possible states of a thread: */
enum tstate { FREE, RUNNING, RUNNABLE };
typedef enum tstate tstate;


struct uthread {
    int                 utid;           // id
    char                ustack[STACK_SIZE];  // the thread's stack
    tstate              state;          // FREE, RUNNING, RUNNABLE
    tcontext            context;        // uswtch() here to run process
    sched_priority      priority;       // scheduling priority
    bool                started;
};
typedef struct uthread userthread;
