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

int
main(int argc, char *argv[])
{
  
  char* inputMessage;
  sendOutput("Hello how can I help you?\n");
  
  while((strcmp((inputMessage = getInput()), "q\n") != 0)  && (strcmp(inputMessage, "Q\n") != 0)){
    sendOutput(inputMessage);
  }

  exit(0);
}
