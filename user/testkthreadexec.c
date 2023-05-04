#include "user.h"


void* fib(){
    int t1=0;
    int t2=1;
    int nextTerm=t1+t2;
    for(int i = 0;i<100;i++){
        t1=t2;
        t2=nextTerm;
        nextTerm=t1+t2;
    }
    printf("fib 100 = %d\n",nextTerm);
    kthread_exit(0);
    return 0;
}
void* exectest(){
    char* command = "echo";
    char *argv[] = {"echo","exec kthread executing\n",0};
    exec(command,argv);
    fprintf(2,"[ERROR] kthread %d has returned\n",kthread_id());
    return 0;
}

void test_kthread_exec(){
    printf("This test will try to call for exec from 3 different threads\nExpected behvaiour: printing Success via a call to exec to run the command echo Success\n");
    void* memory = malloc(4000);
    void* memory2 = malloc(4000);

    int pid = fork();
    if (!pid) kthread_join(kthread_create(exectest,memory,4000), (int*)0);
    else wait((int*)0);

    pid = fork();
    if (!pid) kthread_join(kthread_create(exectest,memory2,4000), (int*)0);
    else wait((int*)0);
    
    

    printf("Success\n");
}