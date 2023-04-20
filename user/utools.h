#include "user/utypes.h"

bool is_debug_on();

void StartDebugMode();

char* PriorityToString(sched_priority p);

int PriorityToInt(sched_priority p);

char* StateToString(tstate t);

void PrintEndLine();

void PrintString(char* function_name_prefix, char*  string_to_print , char* end_string);

void PrintUthread(char* function_name_prefix, userthread* u, char* end_string);

void PrintThreadsFirstToLast(char* function_name_prefix, userthread* first, userthread* last);
