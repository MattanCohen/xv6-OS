#include "user/uthread.h"

//                      CONST NAMES

char* end_line = "\n";
char* function_end = "\n\n";

const bool false  = 0;
const bool true   = 1;

bool is_running(userthread* u) {return u->state == RUNNING;}
bool is_runnable(userthread* u) {return u->state == RUNNABLE;}
bool is_free    (userthread* u) {return u->state == FREE;}



//                  THREAD TABLE

userthread uthread[MAX_UTHREADS];



void PrintAllThreads(char* function_name_prefix){
    PrintThreadsFirstToLast(function_name_prefix, uthread, &uthread[MAX_UTHREADS]);
    PrintEndLine();    
}


//                      UTIDS

int next_utid = 0;
int currentThreadIndex      = -1;
void change_current_thread_index(int newIndex){
    currentThreadIndex = newIndex;
    
}



userthread* uthread_self(){
    char* function_prefix = "uthread_self\t\t";

    PrintString(function_prefix, "the thread table: ", "\n");
    PrintAllThreads(function_prefix);

    userthread* current;
    for (int i = 0; i < MAX_UTHREADS; i++){
        current = &uthread[i];
        if (current->state == RUNNING){
            // change_current_thread_index(i);
            PrintString(function_prefix, "found this thread as running: ", "\n");
            PrintUthread(function_prefix, &uthread[currentThreadIndex], "\n");
            return current;
        }
    }

    PrintString(function_prefix, "no thread is running, returning 0. THIS IS A BUG!", end_line);
    return 0;
}

int debug_threads_created = 0;
int uthread_create(void (*start_func)(), sched_priority priority){

    debug_threads_created++;

    userthread* u;
    for (u = uthread; u < &uthread[MAX_UTHREADS]; u++){
        if (!is_free(u)){
            continue;
        }

        // utid 
        next_utid++;
        u->utid = next_utid;
        // priority
        u->priority = priority;        
        // context || ustack
        memset(&u->context,0, sizeof(tcontext));
        u->context.ra = (uint64)start_func;
        u->context.sp = (uint64)u->ustack;

        // state
        u->state = RUNNABLE;
        
        return 0;
    }
    
    return -1;
}

int index_in_table_from_utid(int utid){
    char* function_prefix = "index_in_table_from_utid";

    PrintString(function_prefix, "searching utid ","");
    if (is_debug_on()) printf("%d from the table :\n", utid);
    PrintAllThreads(function_prefix);

    for (int i = 0; i < MAX_UTHREADS; i++){
        if (uthread[i].utid == utid){
            PrintString(function_prefix, "found thread index ","");
            if (is_debug_on()) printf("%d.\n", i);
            return i;
        }
    }

    PrintString(function_prefix, "cant find utid in table. returning -1!","\n");
    return -1;
}

int find_max_priority_index(int from_index){
    char* function_prefix = "find_max_priority_index ";
    
    PrintString(function_prefix, "looking for the userthread with the maximum priority from this thread table:" ,function_end);
    PrintAllThreads(function_prefix);
    PrintString(function_prefix, "starting the loop from ","");
    if (is_debug_on()) printf("%d\n", from_index);

    from_index = from_index < -1 || from_index > MAX_UTHREADS ? 3 : from_index;
    bool finished = false;
    int i = (from_index + 1) % MAX_UTHREADS;
    int max_priority = -1;
    int max_index = -1;

    while (!finished){
        PrintString(function_prefix, "looping over all procceses, i = ","");
        if (is_debug_on()) printf("%d\n", i);
        //  --code here--
        userthread* u = &uthread[i];
        if ((is_runnable(u) || is_running(u)) && PriorityToInt(u->priority) > max_priority){
            max_priority = PriorityToInt(u->priority);
            max_index = i;        
            PrintString(function_prefix, "found userthread with better priority:" ,end_line);
            PrintUthread(function_prefix, u, end_line);
        }
        //  --code here--


        // end of while (DONT CHANGE)
        if (i == from_index)
            finished = true;
        i = (i + 1) % MAX_UTHREADS;
    }

    PrintString(function_prefix, "chosen userthread (with maximum priority):" ,end_line);
    PrintUthread(function_prefix, &uthread[max_index], function_end);

    PrintString(function_prefix, "index of chosen thread: " ,"");
    if (is_debug_on()) printf("%d\n", max_index);
    
    return max_index;
}

void set_running(userthread* u){
    char* function_prefix = "set_running\t\t";
    
    u->state = RUNNING;
    PrintString(function_prefix, "", "");
    if (is_debug_on()) printf("current thread index before changing: %d\n", currentThreadIndex);
    
    
    change_current_thread_index(index_in_table_from_utid( u->utid));
    
    
    PrintString(function_prefix, "", "");
    if (is_debug_on()) printf("current thread index after changing: %d\n", currentThreadIndex);
}


void uthrun(userthread* u){
    char* function_prefix = "uthrun";
    
    PrintString(function_prefix, "setting next thread to running:", end_line);
    PrintUthread(function_prefix, u, end_line);


    set_running(u);
    

    PrintString(function_prefix, "all threads after change", end_line);
    PrintAllThreads(function_prefix);


    if (!u->started){
        PrintString(function_prefix, "thread runs for the first time", end_line);
        
        u->started = true;  
        void *(*func)();
        func = (void*) u->context.ra;
        func(); 
        return;
    }

    PrintString(function_prefix, "thread ran in the past, switching to exsisting context", end_line);
    uswtch(&uthread_self()->context, &u->context);
}



void set_runnable(userthread* to_change){
    to_change->state = RUNNABLE;
}

void set_free(userthread* to_change){
    to_change->state = FREE;
}

// TODO
void uthread_yield(){
    char* function_prefix = "uthread_yield\t\t"; 

    PrintString(function_prefix, "all threads entering yield", end_line);
    PrintAllThreads(function_prefix);


    userthread* current = uthread_self();
    PrintString(function_prefix, "setting current thread to runnable:", end_line);
    PrintUthread(function_prefix, current, end_line);
    set_runnable(current);


    PrintString(function_prefix, "all threads after change", end_line);
    PrintAllThreads(function_prefix);


    // int nextThreadIndex = find_max_priority_index(index_in_table_from_utid(currentThreadIndex));
    int nextThreadIndex = find_max_priority_index(currentThreadIndex);

    if (nextThreadIndex < 0){
        PrintString(function_prefix, "THIS IS AN ERROR YIELD DIDNT FIND RUNNABLE PROGRAMS", "\n");
        return;
    }
    userthread* u = &uthread[nextThreadIndex];

    uthrun(u);
    return;
}


// TODO
void uthread_exit(){
    char* function_prefix = "uthread_exit\t\t"; 

    PrintString(function_prefix, "process table while entering exit:", end_line);
    PrintAllThreads(function_prefix);

    userthread *current = uthread_self();
    set_free(current);
    PrintString(function_prefix, "changed current thread to free", "\n");
    PrintUthread(function_prefix, current, end_line);
    
    // int nextThreadIndex = find_max_priority_index(index_in_table_from_utid(currentThreadIndex));
    int nextThreadIndex = find_max_priority_index(currentThreadIndex);

    if (nextThreadIndex < 0){
        PrintString(function_prefix, "no runnable threads found in table:", "\n");
        PrintAllThreads(function_prefix);
        PrintString(function_prefix, "finishing program", "\n");
        exit(0);
    }

    userthread* u = &uthread[nextThreadIndex];
    
    uthrun(u);
    return;

}

bool someone_is_running(){
    for (int i = 0; i < MAX_UTHREADS; i++){
        if (is_running(&uthread[i]))
            return true;
    }
    return false;
}


bool already_started_all = false;
int uthread_start_all(){
    char* function_prefix = "uthread_start_all\t"; 

    if (already_started_all)
        return -1;

    already_started_all = true;
    
    PrintString(function_prefix, "all threads in system:" ,end_line);
    PrintAllThreads(function_prefix);
    PrintEndLine();
    
    userthread* u = &uthread[find_max_priority_index(3)];
    PrintString(function_prefix, "user thread with max priority found to run next:" ,end_line);
    PrintUthread(function_prefix, u, function_end);


    uthrun(u);
    // while (someone_is_running()) uthread_exit();
    return 0;
}


// DONE
sched_priority uthread_set_priority(sched_priority priority){
    // char* function_prefix = "uthread_set_priority"; 

    sched_priority old_p = uthread_self()->priority;
    uthread_self()->priority = priority;
    return old_p;
}

// DONE
sched_priority uthread_get_priority(){
    // char* function_prefix = "uthread_get_priority"; 
    
    return uthread_self()->priority;
}
