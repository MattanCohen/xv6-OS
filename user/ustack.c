#include "kernel/types.h"
#include "user/ustack.h"
#include "kernel/riscv.h"
#include "user.h"


static Toy toy;
static Toy *end_toy;

void init_toys(){
    toy.s.size = 0;
    end_toy = &toy;
}
void realloc(char* t){
    Toy* hp = (Toy*)t;
    hp->s.ptr = end_toy;
    hp->s.size = PGSIZE/sizeof(Toy);
    end_toy = hp;
}

void * ustack_malloc(uint len) {
    if(len > 512)
        {return (void*)-1;}
    uint nunits = (len + sizeof(Toy) - 1)/sizeof(Toy) + 1;
    
    if(end_toy == 0) init_toys();
      
    if (end_toy->s.size >= nunits){
        uint remaining_size = end_toy->s.size - nunits;
        end_toy->s.size = nunits;
        end_toy = end_toy + end_toy->s.size;
        end_toy->s.ptr = end_toy-nunits; 
        end_toy->s.size = remaining_size;
        return (void*)(end_toy->s.ptr+1);
    }
    else { // allocate new page and remalloc
        char* new_toy;
        if((new_toy = sbrk(PGSIZE)) == (char*)-1) return (void*)-1;

        realloc(new_toy);
        return ustack_malloc(len);
    }
}


int ustack_free(void) {
    if (end_toy == &toy) // no toys available
        {return -1;}
    Toy* prev_toy = end_toy->s.ptr;
    uint nunits = prev_toy->s.size;
    prev_toy->s.size += end_toy->s.size; 
    end_toy->s.size = 0;     
    end_toy = prev_toy;
    
    if(end_toy->s.size == PGSIZE/sizeof(Toy)) {
        end_toy = end_toy->s.ptr;
        sbrk(-PGSIZE); 
    }
    return nunits*sizeof(Toy);
}