//A Simple C program 
#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/stat.h"
#include "kernel/riscv.h"
#include "kernel/spinlock.h"
#include "kernel/proc.h"
#include "user/user.h"

char* strState(enum procstate state);

//passing command line arguments 
int main(int argc, char *argv[]) 
{

for (int i = 0; i < 2; i++){
    int p1 = fork();
    if(p1 == 0){
        int p2 = fork();
        if (p2 == 0){
            sleep(4);
        }else{
            wait((int *)0);
            sleep(3);
        }
        exit(0);
    }
}
sleep(1);

struct child_processes ch_ps;
int e = chp(&ch_ps);
printf("These are child processes(%d):\nPID\tPPID\tSTATE\tNAME\n", ch_ps.count);
for (int i = 0; i < ch_ps.count; i++){
    printf("%d\t%d\t%s\t%s\n", ch_ps.processes[i].pid, ch_ps.processes[i].ppid, strState(ch_ps.processes[i].state), ch_ps.processes[i].name);
}
wait((int*)0);
wait((int*)0);
exit(e);
return e;
} 

char* strState(enum procstate state){
    switch (state)
    {
    case UNUSED:
        return "UNUSED";
        break;

    case USED:
        return "USED";
        break;


    case SLEEPING:
        return "SLEEPING";
        break;

    case RUNNABLE:
        return "RUNNABLE";
        break;
    
    case RUNNING:
        return "RUNNING";
        break;

    case ZOMBIE:
        return "ZOMBIE";
        break;
    
    default:
        return "UKNOWN";
        break;
    }
}

// This code is contributed by sambhav228 
