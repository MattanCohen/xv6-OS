#include "user/utools.h"

const char* debug_prefix = "$ DEBUG -\t"; 

bool debug = 0;

bool is_debug_on(){return debug;}

void StartDebugMode(){debug = 1;}

char* PriorityToString(sched_priority p){
    switch (p)
    {
    case LOW:
        return "LOW";
    case MEDIUM:
        return "MEDIUM";
    case HIGH:
        return "HIGH";
    }
    return "-1";
}


int PriorityToInt(sched_priority p){
    switch (p)
    {
    case LOW:
        return 0;
    case MEDIUM:
        return 1;
    case HIGH:
        return 2;
    }
    return -1;
}


char* StateToString(tstate t){
    switch (t)
    {
    case FREE:
        return "FREE";
    case RUNNING:
        return "RUNNING";
    case RUNNABLE:
        return "RUNNABLE";
    }
    return "-1";
}
void PrintEndLine(){
    if (debug)
        printf("\n");
}



void PrintString(char* function_name_prefix, char*  string_to_print , char* end_string){
    if (debug)
        printf("%s%s -\t\t%s%s", debug_prefix, function_name_prefix, string_to_print, end_string);

}

void PrintInt   (char* function_name_prefix, int    int_to_print    , char* end_string){
    if (debug)
        printf("%s%s -\t\t%d%s", debug_prefix, function_name_prefix, int_to_print, end_string);

}



void PrintUthread(char* function_name_prefix, userthread* u, char* end_string){
    if (!debug) return;

    printf("%s%s -\t\tuthread #%d: priority = %s, state = %s , context->ra = %d, started? %s%s",  
        debug_prefix, function_name_prefix , u->utid, PriorityToString(u->priority), StateToString(u->state), u->context.ra, u->started ? "true" : "false" , end_string);
}

void PrintThreadsFirstToLast(char* function_name_prefix, userthread* first, userthread* last){
    if (!debug) return;

    userthread* u;
    for (u = first; u < last; u++){
        PrintUthread(function_name_prefix, u, "\n");
    }
}