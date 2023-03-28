#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"



int
main(int argc, char *argv[])
{
  // make sure got enought arguments
  if (argc < 2)
    exit(-1, "policy needs extra argument");
  
  // get new policy from argv
  int new_policy = atoi(argv[1]);

  // make sure argv1 is not string in case of newpolicy = 0
  if (new_policy == 0 && strcmp(argv[1], "0") != 0)
    exit(-1, "policy cant change to string");

  set_policy(new_policy);

  exit(0, "policy normal exit");
}
