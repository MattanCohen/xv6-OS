#include "user/seek_test_data.c"

#define run_test1(f, p) printf("~~~~~~~~~~~ TEST FUNCTION %s: %s ! ~~~~~~~~~~~\n\n", #f, (f)((int)(p)) ? "SUCCEED" : "FAILED")
#define run_test3(f, r, p) printf("~~~~~~~~~~~ TEST FUNCTION %s: %s ! ~~~~~~~~~~~\n\n", #f, (f)((int)(r), (char**)(p)) ? "SUCCEED" : "FAILED")

int
main(int argc, char *argv[])
{
  int debug = 1;
 
  int run_test_random             = 1;
  int run_test_ass4               = 1;

  if(run_test_ass4) run_test3(test_ass4, argc, argv);
  if(run_test_random) run_test1(test_random, debug);

  return 0;
}