#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "user/uthread.h"

//          tools for tests ( ALL ARE IN THE BOTTOM OF THE PAGE)
void uthread_print();
void change_priority();


//      TEST 1 - priority_print
void test1(){
    char* priority = "";
    switch (uthread_get_priority())
    {
    case LOW:
        priority = "LOW";
        break;
    case MEDIUM:
        priority = "MEDIUM";
        break;
    case HIGH:
        priority = "HIGH";
        break;
    }
    printf("%s before yield\n", priority);
    uthread_yield();
    printf("%s after yield\n", priority);
    uthread_exit();
}


//      TEST 2 print_me
void test2(){
    uthread_print();
    uthread_yield();
    uthread_print();
    uthread_exit();
}


//      TEST 3 loop
void test3(){
    int num_of_prints = 4;
    for (int i = 0; i < num_of_prints; i++){
        uthread_print();
        uthread_yield();
    }
    uthread_exit();
}

//      TEST 4 priority_change
void test4(){
    int num_of_loops = 4;
    for (int i = 0; i < num_of_loops; i++){
        uthread_print();
        uthread_yield();

        change_priority();
        
        uthread_print();
        uthread_yield();
    }
    uthread_exit();
}

void test5(){
    switch (PriorityToInt(uthread_self()->priority))
    {
    case HIGH:
        uthread_create(&test5, LOW);
        uthread_exit();
    case MEDIUM:
        uthread_exit();
    default:
        break;
    }

    uthread_print();
    uthread_yield();
    uthread_exit();
}





// ------------------------------------------------------------------------------------------------------------------------

// ------------------------------------------------  tools ----------------------------------------------------------------

// ------------------------------------------------------------------------------------------------------------------------

void uthread_print(){
    userthread* u = uthread_self();
    printf("uthread #%d: priority = %s, state = %s , context->ra = %d\n",  
        u->utid, PriorityToString(u->priority), StateToString(u->state), u->context.ra);
}

void change_priority(){
    switch (uthread_get_priority())
    {
    case HIGH:
        uthread_set_priority(LOW);
        break;
    case MEDIUM:
        uthread_set_priority(HIGH);
        break;
    case LOW:
        uthread_set_priority(MEDIUM);
        break;
    }
}
