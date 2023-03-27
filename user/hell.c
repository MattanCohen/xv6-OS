#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


void sendOutput(char* message){
  write(1, message, strlen(message));
}

char* getInput(){
  char * inputMessage = malloc(sizeof(char) * 60);
  gets(inputMessage, sizeof(inputMessage));
  return inputMessage;
}

int TestFunction(){
  int test1 = 1;

  if (test1){
    if (fork()){
      set_ps_priority(1);
      if(!fork()){
        exit(0, "child 0");
      }
      else
      {
       if(!fork()){
        set_ps_priority(2);
        exit(0, "child 1");
      } 
        set_ps_priority(3);
      }
      set_ps_priority(4);
    }
  }
  
  else{
    if (!fork()){
      set_ps_priority(1);
      if (!fork()){
        set_ps_priority(2);
        if (!fork()){
          set_ps_priority(3);
          if (!fork()){
            set_ps_priority(4);
            if (!fork()){
              set_ps_priority(5);
            }  
          }  
        }  
      }
    }
  }

  exit(0, "hell - test function");
}


int
main(int argc, char *argv[])
{
  if (argc > 1 && strcmp(argv[1], "test") == 0)
    return TestFunction();
  
  char* inputMessage;
  sendOutput("Hello how can I help you?\n");

  while((strcmp((inputMessage = getInput()), "q\n") != 0)  && (strcmp(inputMessage, "Q\n") != 0)){
    sendOutput(inputMessage);
  }

  exit(0, "hell normal exit");
}
