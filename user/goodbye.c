#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"



int
main(int argc, char *argv[])
{
  int pid;
  if(! (pid =fork())){
    //child process 
    
    exit(55, "Goodbye World xv6");
  }
  else{
    //parent process
    int status;
    char message[32];
    
    wait(&status, (char**)&message);
  printf("@@ - in parent recieved from child proccess (pid #%d):\n  status - %d,\n  message - %s.\n", pid, status, message);
  }

  exit(0, "goodbye command normal exit") ;
}
