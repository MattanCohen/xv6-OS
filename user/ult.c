#include "ult_tools.c"


void create_thread1(sched_priority p){
    uthread_create(&priority_print, p);
}

void create_thread2(sched_priority p){
    uthread_create(&print, p);
}

void create_thread3(sched_priority p){
    uthread_create(&loop, p);
}

void create_thread4(sched_priority p){
    uthread_create(&priority_change, p);
}

int main(int argc, char *argv[])
{
    // if no num of threads were chosen, pick 1
    int num_of_threads = 1;

    sched_priority priorities [] = {HIGH, MEDIUM, MEDIUM, LOW};
    
    bool test1 = 0;
    bool test2 = 0;
    bool test3 = 0;
    bool test4 = 0;
    
    for (int i = 1; i < argc; i++){
        int argv_atoi = atoi(argv[i]);
        if (num_of_threads < MAX_UTHREADS && num_of_threads < argv_atoi)
            num_of_threads = argv_atoi;

        if (strcmp(argv[i], "d") == 0 || strcmp(argv[i], "D") == 0) StartDebugMode();
        if (strcmp(argv[i], "test1") == 0 || strcmp(argv[i], "priority_print") == 0)    test1 = 1;
        if (strcmp(argv[i], "test2") == 0 || strcmp(argv[i], "prints") == 0)            test2 = 1;
        if (strcmp(argv[i], "test3") == 0 || strcmp(argv[i], "loops") == 0)             test3 = 1;
        if (strcmp(argv[i], "test4") == 0 || strcmp(argv[i], "priority_change") == 0)   test4 = 1;
    }

    // if no test was chosen, light test1
    if (test1 + test2 + test3 + test4 == 0)
        test1 = 1;

    for (int i = 0; i < num_of_threads; i++){
        if (test1)
            create_thread1(priorities[i % (sizeof(priorities) / sizeof(sched_priority))]);
        if (test2)
            create_thread2(priorities[i % (sizeof(priorities) / sizeof(sched_priority))]);
        if (test3)
            create_thread3(priorities[i % (sizeof(priorities) / sizeof(sched_priority))]);
        if (test4)
            create_thread4(priorities[i % (sizeof(priorities) / sizeof(sched_priority))]);
    }


    uthread_start_all();

    printf("ADDITIONAL LINES AFTER START ALL :\n");
    printf("\tthose lines should never be printed\n");
    printf("\tif those lines were printed please check your code.\n");

    return -1;
}

/**
 * 
 * ANSWERS ANSWERS ANSWERS ANSWERS ANSWERS ANSWERS ANSWERS ANSWERS ANSWERS ANSWERS ANSWERS              
 * 
 $ test1 :
        hart 2 starting
        hart 1 starting
        init: starting sh
        $ ult
        HIGH before yield
        HIGH after yield
        $ ult 2 
        HIGH before yield
        HIGH after yield
        MEDIUM before yield
        MEDIUM after yield
        $ ult 3
        HIGH before yield
        HIGH after yield
        MEDIUM before yield
        MEDIUM before yield
        MEDIUM after yield
        MEDIUM after yield
        $ ult 4
        HIGH before yield
        HIGH after yield
        MEDIUM before yield
        MEDIUM before yield
        MEDIUM after yield
        MEDIUM after yield
        LOW before yield
        LOW after yield 


 $ test2 :
        $ ult test2 
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 224
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 5000
        $ ult test2 2
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 224
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 5000
        uthread #2: priority = MEDIUM, state = RUNNING , context->ra = 224
        uthread #2: priority = MEDIUM, state = RUNNING , context->ra = 5000
        $ ult test2 3
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 224
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 5000
        uthread #2: priority = MEDIUM, state = RUNNING , context->ra = 224
        uthread #3: priority = MEDIUM, state = RUNNING , context->ra = 224
        uthread #2: priority = MEDIUM, state = RUNNING , context->ra = 5000
        uthread #3: priority = MEDIUM, state = RUNNING , context->ra = 5000
        $ ult test2 4
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 224
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 5000
        uthread #2: priority = MEDIUM, state = RUNNING , context->ra = 224
        uthread #3: priority = MEDIUM, state = RUNNING , context->ra = 224
        uthread #2: priority = MEDIUM, state = RUNNING , context->ra = 5000
        uthread #3: priority = MEDIUM, state = RUNNING , context->ra = 5000
        uthread #4: priority = LOW, state = RUNNING , context->ra = 224
        uthread #4: priority = LOW, state = RUNNING , context->ra = 5000

 $ test3 :
        
        hart 2 starting
        hart 1 starting
        init: starting sh
        $ ult test3 
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 272
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 5038
        $ ult test3 2
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 272
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #2: priority = MEDIUM, state = RUNNING , context->ra = 272
        uthread #2: priority = MEDIUM, state = RUNNING , context->ra = 5038
        uthread #2: priority = MEDIUM, state = RUNNING , context->ra = 5038
        uthread #2: priority = MEDIUM, state = RUNNING , context->ra = 5038
        $ ult test3 3
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 272
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #2: priority = MEDIUM, state = RUNNING , context->ra = 272
        uthread #3: priority = MEDIUM, state = RUNNING , context->ra = 272
        uthread #2: priority = MEDIUM, state = RUNNING , context->ra = 5038
        uthread #3: priority = MEDIUM, state = RUNNING , context->ra = 5038
        uthread #2: priority = MEDIUM, state = RUNNING , context->ra = 5038
        uthread #2: priority = MEDIUM, state = RUNNING , context->ra = 5038
        uthread #2: priority = MEDIUM, state = RUNNING , context->ra = 5038
        uthread #2: priority = MEDIUM, state = RUNNING , context->ra = 5038
        $ ult test3 4
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 272
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #2: priority = MEDIUM, state = RUNNING , context->ra = 272
        uthread #3: priority = MEDIUM, state = RUNNING , context->ra = 272
        uthread #2: priority = MEDIUM, state = RUNNING , context->ra = 5038
        uthread #3: priority = MEDIUM, state = RUNNING , context->ra = 5038
        uthread #2: priority = MEDIUM, state = RUNNING , context->ra = 5038
        uthread #2: priority = MEDIUM, state = RUNNING , context->ra = 5038
        uthread #2: priority = MEDIUM, state = RUNNING , context->ra = 5038
        uthread #2: priority = MEDIUM, state = RUNNING , context->ra = 5038
        uthread #4: priority = LOW, state = RUNNING , context->ra = 272
        uthread #4: priority = LOW, state = RUNNING , context->ra = 5038
        uthread #4: priority = LOW, state = RUNNING , context->ra = 5038
        uthread #4: priority = LOW, state = RUNNING , context->ra = 5038


 $ test4 :
        hart 1 starting
        hart 2 starting
        init: starting sh
        $ ult test4
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 436
        uthread #1: priority = LOW, state = RUNNING , context->ra = 5038
        uthread #1: priority = LOW, state = RUNNING , context->ra = 5038
        uthread #1: priority = MEDIUM, state = RUNNING , context->ra = 5038
        uthread #1: priority = MEDIUM, state = RUNNING , context->ra = 5038
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #1: priority = LOW, state = RUNNING , context->ra = 5038
        $ ult test4 2
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 436
        uthread #1: priority = LOW, state = RUNNING , context->ra = 5038
        uthread #2: priority = MEDIUM, state = RUNNING , context->ra = 436
        uthread #2: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #2: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #2: priority = LOW, state = RUNNING , context->ra = 5038
        uthread #1: priority = LOW, state = RUNNING , context->ra = 5038
        uthread #2: priority = MEDIUM, state = RUNNING , context->ra = 5038
        uthread #2: priority = MEDIUM, state = RUNNING , context->ra = 5038
        uthread #2: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #1: priority = LOW, state = RUNNING , context->ra = 5038
        uthread #1: priority = MEDIUM, state = RUNNING , context->ra = 5038
        uthread #1: priority = MEDIUM, state = RUNNING , context->ra = 5038
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #1: priority = LOW, state = RUNNING , context->ra = 5038
        $ ult test4 3
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 436
        uthread #1: priority = LOW, state = RUNNING , context->ra = 5038
        uthread #2: priority = MEDIUM, state = RUNNING , context->ra = 436
        uthread #3: priority = MEDIUM, state = RUNNING , context->ra = 436
        uthread #2: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #2: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #2: priority = LOW, state = RUNNING , context->ra = 5038
        uthread #3: priority = MEDIUM, state = RUNNING , context->ra = 5038
        uthread #3: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #3: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #3: priority = LOW, state = RUNNING , context->ra = 5038
        uthread #2: priority = MEDIUM, state = RUNNING , context->ra = 5038
        uthread #2: priority = MEDIUM, state = RUNNING , context->ra = 5038
        uthread #2: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #2: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #2: priority = LOW, state = RUNNING , context->ra = 5038
        uthread #3: priority = LOW, state = RUNNING , context->ra = 5038
        uthread #2: priority = MEDIUM, state = RUNNING , context->ra = 5038
        uthread #3: priority = LOW, state = RUNNING , context->ra = 5038
        uthread #3: priority = MEDIUM, state = RUNNING , context->ra = 5038
        uthread #3: priority = MEDIUM, state = RUNNING , context->ra = 5038
        uthread #3: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #3: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #3: priority = LOW, state = RUNNING , context->ra = 5038
        $ ult test4 4
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 436
        uthread #1: priority = LOW, state = RUNNING , context->ra = 5038
        uthread #2: priority = MEDIUM, state = RUNNING , context->ra = 436
        uthread #3: priority = MEDIUM, state = RUNNING , context->ra = 436
        uthread #2: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #2: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #2: priority = LOW, state = RUNNING , context->ra = 5038
        uthread #3: priority = MEDIUM, state = RUNNING , context->ra = 5038
        uthread #3: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #3: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #3: priority = LOW, state = RUNNING , context->ra = 5038
        uthread #4: priority = LOW, state = RUNNING , context->ra = 436
        uthread #1: priority = MEDIUM, state = RUNNING , context->ra = 5038
        uthread #1: priority = MEDIUM, state = RUNNING , context->ra = 5038
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #1: priority = LOW, state = RUNNING , context->ra = 5038
        uthread #2: priority = LOW, state = RUNNING , context->ra = 5038
        uthread #3: priority = MEDIUM, state = RUNNING , context->ra = 5038
        uthread #1: priority = MEDIUM, state = RUNNING , context->ra = 5038
        uthread #1: priority = MEDIUM, state = RUNNING , context->ra = 5038
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #1: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #1: priority = LOW, state = RUNNING , context->ra = 5038
        uthread #2: priority = LOW, state = RUNNING , context->ra = 5038
        uthread #1: priority = MEDIUM, state = RUNNING , context->ra = 5038
        uthread #2: priority = LOW, state = RUNNING , context->ra = 5038
        uthread #2: priority = MEDIUM, state = RUNNING , context->ra = 5038
        uthread #2: priority = MEDIUM, state = RUNNING , context->ra = 5038
        uthread #2: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #2: priority = HIGH, state = RUNNING , context->ra = 5038
        uthread #2: priority = LOW, state = RUNNING , context->ra = 5038
*/