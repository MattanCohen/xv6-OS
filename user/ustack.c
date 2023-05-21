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
    char* toySbrk = sbrk(len * sizeof(union node));
    if (toySbrk == (char*)-1) return (void*)-1;
    toy newToy = (toy)toySbrk;
    
    // assign new toy size
    newToy->size = len;

    // if its the first toy
    if (startToy == 0){
        newToy->prev = 0;
        startToy = newToy;
        lastToy = startToy;
    }
    else{
        newToy->prev = lastToy;
        lastToy = newToy;
    }
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