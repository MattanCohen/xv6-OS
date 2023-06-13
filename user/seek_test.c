#include "user/seek_test_data.c"

#define run_test1(f, p) printf("function %s: %s\n", #f, (f)((int)(p)) ? "SUCCEED" : "FAILED")
#define run_test2(f, r, p) printf("function %s: %s\n", #f, (f)((int)(r), (int)(p)) ? "SUCCEED" : "FAILED")

int
main(int argc, char *argv[])
{
  int print = 1;
  // int print = argc > 1 && !(strcmp(argv[1], "r") == 0 || strcmp(argv[1], "reset") == 0) ;
  
  // int reset = argc > 1 && (strcmp(argv[1], "r") == 0 || strcmp(argv[1], "reset") == 0) ;


  // run_test1(test_seek, print);
  run_test1(test_random_m,print);

  // run_test2(test_random, reset, print);

  return 0;
}