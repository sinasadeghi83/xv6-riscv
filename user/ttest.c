//A Simple C program 
#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/stat.h"
#include "kernel/riscv.h"
#include "kernel/spinlock.h"
#include "kernel/proc.h"
#include "user/user.h"

volatile int a = 0;

void mt(void *arg){
    printf("THIS IS SECOND THREAD\n");
    // int *number = arg;
    // for(int i = 0; i<10; ++i){
        // *number = *number + 1;

        // if(number == &a){
        //     printf("thread a: %d\n", *number);
        // }else if(number == &b){
        //     printf("thread b: %d\n", *number);
        // }else{
        //     printf("thread c: %d\n", *number);
        // }
    // }
    exit(0);
}

//passing command line arguments 
int main(int argc, char *argv[]) 
{
    printf("THIS IS MAIN THREAD\n");
    void *stacka = malloc(PGSIZE);
    // void *stackb = malloc(PGSIZE);
    // void *stackc = malloc(PGSIZE);
    thread_create(mt, (void *)&a, stacka);
    // thread_create(mt, (void *)&b, stackb);
    // thread_create(mt, (void *)&c, stackc);
    exit(0);
} 
