#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "user/testkthreadexec.c"
#include "user/testkthreadfork.c"
#include "user/testkthreadkilljoin.c"



#define MAX_STACK_SIZE 4000

struct test {
  void (*f)();
  char *s;
} tests [] = {
  {test_kthread_fork, "test kthread fork"},
  {test_kthread_kill_join, "test kthread kill join"},
  {test_kthread_exec, "test kthread exec"},
  
  {0, 0},
};

void starting_tests(){
    printf("\n♥\t-\tstarting all tests\n");
}

void performing_test(struct test* test){
    printf("\n------------------- starting test \"%s\" -------------------\n", test->s);
    test->f();
    printf("------------------------------------------------------------\n");
}

void finished_tests(){
    printf("\n♥\t-\tfinished all tests\n");
}


int
main(int argc, char *argv[])
{


    starting_tests();

    int done = 0;
    for (struct test* test = tests; test < &tests[9999] && !done; test++)
    {
        if (tests->s || !tests->f){
            done = 1;
        }
        performing_test(test);
    }
    

    finished_tests();
    
    exit(0);
}


