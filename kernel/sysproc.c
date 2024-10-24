#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

//returns children and grand childrens of a process
uint64
sys_chp(void)
{
  struct child_processes* children;
  struct child_processes result_children;
  argaddr(0, (uint64 *)&children);

  int e = chp(&result_children);

  struct proc *p = myproc();

  if(copyout(p->pagetable, (uint64)children, (char*)&result_children, sizeof(result_children)) < 0){
    return -1;
  }
  return e;
}

//returns trap reports of children and grand childrens of a process
uint64
sys_trprp(void)
{
  struct report_traps* reports;
  struct report_traps result_reports;
  argaddr(0, (uint64 *)&reports);

  int e = trprp(&result_reports);

  struct proc *p = myproc();

  if(copyout(p->pagetable, (uint64)reports, (char*)&result_reports, sizeof(result_reports)) < 0){
    return -1;
  }
  return e;
}