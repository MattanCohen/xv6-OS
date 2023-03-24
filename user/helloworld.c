#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


void sendOutput(char* message){
  write(1, message, strlen(message));
}


int
main(int argc, char *argv[])
{
  
  sendOutput("Hello world xv6");

  exit(0, "helloworld command normal exit");
}
