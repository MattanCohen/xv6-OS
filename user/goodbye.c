#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"



int
main(int argc, char *argv[])
{
  if(!fork()){
    //child process 
    printf("child started running\n");
    exitwithmsg(55, "Goodbye World xv6");
  }
  else{
    //parent process 
    int status;
    char message[32];
    char* msg = message;
    
    waitwithmsg(&status, &msg);
    printf("\nparent recieved: \n\tstatus: %d \n\tmessage: %s\nexiting program.\n", status, message);
  }

  exit(0) ;
}
