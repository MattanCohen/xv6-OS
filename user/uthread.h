#include "user/utools.h"


extern void uswtch(struct context*, struct context*);

int uthread_create(void (*start_func)(), enum sched_priority priority);

void uthread_yield();

void uthread_exit();

int uthread_start_all();

enum sched_priority uthread_set_priority(enum sched_priority priority);

enum sched_priority uthread_get_priority();

struct uthread* uthread_self();

void uthrun(userthread* u);
