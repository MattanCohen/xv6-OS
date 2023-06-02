#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "user/ustack.h"

void test_malloc(int isMyMalloc) {

    printf("-----------------TESTING %s-----------------\n", isMyMalloc ? "USTACK_MALLOC" : "MALLOC");

    int numOfChars = 511;
    // Allocate memory blocks of different sizes
    int *int_ptr =        isMyMalloc ? (int*)ustack_malloc(sizeof(int))         : (int*)malloc(sizeof(int));
    char *char_ptr =      isMyMalloc ? (char*)ustack_malloc(numOfChars * sizeof(char))  : (char*)malloc(numOfChars * sizeof(char));

    // Store data in the allocated memory blocks
    *int_ptr = 42;

    int i = 0;
    while (i < numOfChars)
    {
      char_ptr[i] = ' ';
      if (i % 100 == 0) char_ptr[i] = 'X';
      i++;
    }
    char_ptr[numOfChars - 1] = '\0';

    // Print the stored data
    printf("int: %d (supposed to be 42)\n", *int_ptr);
    printf("char: ");
    for (int i = 0; i < numOfChars; i++)
    {
      if (char_ptr[i] == 'X') printf("X");
    }
    printf("\n");    
    printf("if printed XXXXXX so good job else you suck yes\n");    

    // Free the allocated memory
    if (!isMyMalloc) free(int_ptr);
    else            ustack_free();
    if (!isMyMalloc) free(char_ptr);
    else            ustack_free();

    // printf("you tried freeing the pointers and they ");
    
    // if (int_ptr == 0 && char_ptr == 0) printf("freed successfuly !!!\n");
    // else                               printf("stayed like a bitch...\n");

    printf("-------------------------------------------------\n");
}


void crash_malloc(int isMyMalloc) {

    printf("-----------------CRASHING %s-----------------\n", isMyMalloc ? "USTACK_MALLOC" : "MALLOC");

    int numOfChars = 511;

    char *char_ptr =      isMyMalloc ? (char*)ustack_malloc(1)  : (char*)malloc(1);

    // Store data in the allocated memory blocks
    int i = 0;
    // while (i < numOfChars)
    while (1)
    {
      char_ptr[i] = ' ';
      if (i % 100 == 0) char_ptr[i] = 'X';
      i++;
    }
    char_ptr[numOfChars - 1] = '\0';


    // Print the stored data
    printf("char: ");
    for (int i = 0; i < numOfChars; i++)
    {
      if (char_ptr[i] == 'X') printf("X");
    }
    printf("\n");    
    printf("if reached here, your malloc sucks ass\n");    

    // Free the allocated memory
    if (isMyMalloc) free(char_ptr);
    else            ustack_free();

    printf("-------------------------------------------------\n");
}

void combine_mallocs(){
  
    printf("-----------------COMBINIG %s and %s-----------------\n" , "MALLOC","USTACK_MALLOC");

    int numOfChars = 511;

    char *ustack_char_ptr = (char*)ustack_malloc(numOfChars * sizeof(char));
    char *char_ptr =        (char*)malloc(numOfChars * sizeof(char));

    // Store data in the allocated memory blocks
    int i = 0;
    while (i < numOfChars)
    {
      char_ptr[i] = ' ';
      ustack_char_ptr[i] = ' ';
      if (i % 100 == 0){

        char_ptr[i] = 'X';
        ustack_char_ptr[i] = 'X';
      } 
      i++;
    }
    char_ptr[numOfChars - 1] = '\0';
    ustack_char_ptr[numOfChars - 1] = '\0';


    // Print the stored data
    printf("char: ");
    for (int i = 0; i < numOfChars; i++)
    {
      if (char_ptr[i] == 'X' && ustack_char_ptr[i] == 'X') printf("X");
    }
    printf("\n");    
    printf("if printed XXXXXX so good job else you suck yes\n");    
    free(char_ptr);
    ustack_free();

    printf("-------------------------------------------------\n");
}

int
main(int argc, char *argv[])
{
  if (argc > 1 && strcmp(argv[1], "quick") == 0){
    test_malloc(0);
    test_malloc(1);
    int c;
    if ((c = fork()) == 0) crash_malloc(0);
    sleep(c);
    if ((c = fork()) == 0) crash_malloc(1);
    sleep(c);
    combine_mallocs();
    exit(0);
  }


  // supposed results with normal malloc
  if (argc > 1 && strcmp(argv[1], "normal") == 0){
    if (argc > 2 && strcmp(argv[2], "crash") == 0)
      crash_malloc(0);
    else
      test_malloc(0);
  }
  // results with ustack malloc
  else{
    if (argc > 1 && strcmp(argv[1], "crash") == 0)
      crash_malloc(1);
    else
      test_malloc(1);
  }

  if (argc > 3){
    combine_mallocs();
  }

  exit(0);
}


int
main2(int argc, char *argv[])
{
    printf("helloworld\n");

    //fork test
    int pid = fork();
    if(pid == 0){
        printf("child\n");
 
        int mallo = 20;
        char *arr[mallo];
        for(int i = 0; i < mallo; i++){
            char *a = malloc(4096);
            //fill in the memory with some data
            //keep a in an array
            arr[i] = a;

            for(int j = 0; j < 4096; j++){
                a[j] = 'a';
            }
            printf("malloc 4096, i: %d\n", i);
        }

        //free the memory
        for(int i = 0; i < mallo; i++){
            free(arr[i]);
        } 
        printf("free 4096\n");

    }
    else{
        printf("parent\n");
        wait(0);
    }
    
exit(0);
}
