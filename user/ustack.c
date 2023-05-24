#include "ustack.h"

union node
{
    union node* prev;
    uint size;
};
typedef union node* toy;

toy startToy;
toy lastToy;

void*
ustack_malloc (uint len){
    if (len > MAXTOYLEN || len == 0) return (void*)-1; // len more than allowed

    // create new toy
    char* toySbrk = sbrk(sizeof(union node));
    if (toySbrk == (char*)-1) return (void*)-1;
    
    toy newToy = (toy)toySbrk; 
    // assign new toy size
    newToy->size = len;

    // if its the first toy
    if (startToy == 0){
        newToy->prev = 0;
        startToy = newToy;
        lastToy = newToy;
    }
    else{
        newToy->prev = lastToy;
        lastToy = newToy;
    }
    
    // printf("created new toy so now last's len is: %d while newToys size is %d although len=%d\n", lastToy->size, newToy->size, len);
    return (void*)(newToy);
}


int     
ustack_free(void){
    if (lastToy == 0) return -1;
    uint len = lastToy->size;
    if (sbrk(-1 * len * sizeof(toy))==(char*)-1) return -1;

    toy prevToy = lastToy;
    lastToy = prevToy ? prevToy : 0;
    if (prevToy){ // Tidy up
        prevToy->prev = 0;
        prevToy->size = 0;
    }
    return len;
}