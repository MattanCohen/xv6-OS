#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include <inttypes.h>
// #include <string.h>  

 
// void tostring(char str[], int num,/ char suffix)
void tostring(char str[], int num)
{
    int i, rem, len = 0, n;
 
    n = num;
    while (n != 0)
    {
        len++;
        n /= 10;
    }
    for (i = 0; i < len; i++)
    {
        rem = num % 10;
        num = num / 10;
        str[len - (i + 1)] = rem + '0';
    }

        // str[len] = suffix;
        // str[len + 1] = '\0';
        str[len] = '\0';
}

void ssendOutput(char* message){
    write(1, message, strlen(message));
}

void isendOutput(int n){
    char message[64];
    // tostring(message, n, '\0');
    tostring(message, n);
    write(1, message, strlen(message));
}


void memsizeTestPrint(char* outputMessage){
    char sysCallOutput[10];
    tostring(sysCallOutput, memsize());

    ssendOutput(outputMessage);
    ssendOutput(sysCallOutput);
    ssendOutput("\n");
}



int main(int argc, char *argv[])
{
    memsizeTestPrint("the size of the memory before calling malloc is: ");
    void* a = malloc(20000);
    memsizeTestPrint("the size of the memory after calling malloc is: ");
    free(a);
    memsizeTestPrint("the size of the memory after freeing the array is: ");
    
    exit(0);
}