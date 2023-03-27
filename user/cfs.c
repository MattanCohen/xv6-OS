#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void iter(int prio){
  set_cfs_priority(prio);
  for (int i = 0; i < 1000000; i++){
    if (i % 100000 != 0) continue;
    sleep(1);
  }
}
void print_arr(int i, int* arr){
  printf("child #%d : \n\tcfs priorty = %d\n\trtime = %d\n\tstime = %d\n\tretime = %d\n", i, arr[0], arr[1], arr[2], arr[3]);
}

int
main(int argc, char *argv[])
{
  int pid1, pid2, pid3;
  pid1 = fork();
  int stats_pid1[4];
  int stats_pid2[4];
  int stats_pid3[4];
  
  if (!pid1) {
    iter(0);
    get_cfs_priority(getpid(), &stats_pid1[0], &stats_pid1[1], &stats_pid1[2], &stats_pid1[3]);
      int* arr = stats_pid1;
      print_arr(getpid(), arr);
      
    }
  else
  {
    pid2 = fork();
    if (!pid2) {
      iter(1);
      get_cfs_priority(getpid(), &stats_pid2[0], &stats_pid2[1], &stats_pid2[2], &stats_pid2[3]);
      int* arr = stats_pid2;
      sleep(5);
      print_arr(getpid(), arr);
  
  }
    else
    {
      pid3 = fork();
      if (!pid3) {
        iter(2);
        get_cfs_priority(getpid(), &stats_pid3[0], &stats_pid3[1], &stats_pid3[2], &stats_pid3[3]);
        int* arr = stats_pid3;
        sleep(10);
        print_arr(getpid(), arr);
        }
    }
  }


  exit(0, "cfs command normal exit");
}
