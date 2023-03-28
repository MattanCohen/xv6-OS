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

int TestMaleForkim(){
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

void loopon(){
  for (int i = 0; i < 50000; i++)
  {
    int x = i + 5000000;
    int y = x + 999;
    x = y + i;
  }
}

int TestFunction(){
  
  int pid = getpid();

  int c0; int r0;int  s0;int  re0;
  int c1;int  r1;int  s1;int  re1;
  int c2;int  r2;int  s2;int  re2;

  set_policy(0);
  loopon();
  get_cfs_priority(pid, &c0, &r0, &s0, &re0);
  
  set_policy(1);
  loopon();
  get_cfs_priority(pid, &c1, &r1, &s1, &re1);

  set_policy(2);
  loopon();
  get_cfs_priority(pid, &c2, &r2,&s2, &re2);

  printf("\n\n");
  

  exit (0, "hell - test function");
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
