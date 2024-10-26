// A Simple C program
#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/stat.h"
#include "kernel/riscv.h"
#include "kernel/spinlock.h"
#include "kernel/proc.h"
#include "user/user.h"

char *strState(enum procstate state);

// passing command line arguments
int main(int argc, char *argv[])
{
    for (int i = 0; i < 1; i++)
    {
        int p1 = fork();
        if (p1 == 0)
        {
            int p2 = fork();
            if (p2 == 0)
            {
                int *x = 0;
                *x = 4;
            }
            else
            {
                sleep(1);
                // printf("i:%d\tsecond pid: %d\n", i, p2);
                int *x = 0;
                *x = 4;
                // wait((int *)0);
            }
            exit(0);
        }
        sleep(3);
        // printf("i:%d\tfirst pid: %d\n", i, p1);

    }
    sleep(5);

    struct report_traps rps;
    int e = trprp(&rps);
    printf("These are child processes traps(%d):\nPID\tPNAME\tscause\tsepc\tstval\n", rps.count);
    for (struct report *rp = rps.reports; rp < &rps.reports[rps.count]; rp++)
    {
        printf("%d\t%s\t%ld\t%ld\t%ld\n", rp->pid, rp->pname, rp->scause, rp->sepc, rp->stval);
    }
    wait((int *)0);
    wait((int *)0);
    exit(e);
    // return e;
}
