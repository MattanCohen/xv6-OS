#include "user.h"


void chld(){
    printf("Just a perfect day, drink Sangria in the park\
    \nAnd then later, when it gets dark, we go home\n\
Just a perfect day, feed animals in the zoo\n\
Then later, a movie too and then home\n\n\
Oh, it's such a perfect day\n\
I'm glad I spent it with you\n\
Oh, such a perfect day\n\
You just keep me hanging on\n\
You just keep me hanging on\n\n\
Just a perfect day, problems all left alone\n\
Weekenders on our own, it's such fun\n\
Just a perfect day, you made me forget myself\n\
I thought I was someone else, someone good\n\n\
Oh, it's such a perfect day\n\
I'm glad I spent it with you\n\
Oh, such a perfect day\n\
You just keep me hanging on\n\
You just keep me hanging on\n");
exit(1);
}
void* thread(){
    int status;
    int pid;
    printf("hi\n");
    if((pid = fork()) ==0){
        printf("hi2\n");
        fprintf(2,"child's PID: %d\n",getpid());
        chld();
    }else{
        printf("hi3\n");
        int wait_val;
        if((wait_val = wait(&status)) != -1 && status != 1){
                printf("fork status %d \n\twait_val : %d\n", status , wait_val); 
                printf("hi4\n");
                fprintf(2,"[ERROR] child exited abnormally\n");   
                kthread_exit(-1);         
        }
        printf("hi5\n");
        kthread_exit(0);
    }
    return 0;
}

void test_kthread_fork(){
    printf("This test will try to fork this process from a thread.\n\
    Expected Behaviuor:\n\
    the child prints the lyrics of the song Perfect Day by Lou Reed and exit\n\n");
    fprintf(2,"father PID: %d\n",getpid());
    void* stack = malloc(4000);
    int tid;
    if((tid = kthread_create(thread,stack,4000)) == -1){
        fprintf(2,"[ERROR] couldn't start a thread\n");
    }
    
    kthread_join(tid, (int*)0);
    printf("Success\n");
}