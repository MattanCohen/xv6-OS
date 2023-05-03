#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MAX_STACK_SIZE 4000


void kthread_start_func(void){
    printf("☺☻☺☻☺ Hi! ☺☻☺☻☺ \n");
    kthread_exit(kthread_id());
}

int
main(int argc, char *argv[])
{
    int xstatus = -1;
    uint64 stack = (uint64)malloc(MAX_STACK_SIZE);

    int kt = kthread_create((void *(*)())kthread_start_func, (void*)stack, MAX_STACK_SIZE);
    kthread_join(kt, &xstatus);

    printf("finished klt #%d !!! xstatus = %d\n", kt, xstatus);
    exit(0);
    
}


