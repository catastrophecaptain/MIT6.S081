#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
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


#ifdef LAB_PGTBL
int
sys_pgaccess(void)
{
  // lab pgtbl: your code here.
  uint32 mask=0;
  uint64 raddr=0;
  int n=0;
  uint64 pg=0;
  pte_t * pte=0;
  argaddr(0,&pg);
  argint(1,&n);
  argaddr(2,&raddr);

  if(n>32)
  {
    return -1;
  }
  n--;
  pg+=n*PGSIZE;
  for(;n>=0;n--)
  {
    pte=walk(myproc()->pagetable,pg,0);
    if(pte!=0&&(*pte&PTE_A)!=0)
    {
      mask|=(1<<n);
      *pte=*pte&(~PTE_A);
    }
    pg-=PGSIZE;
  }
  copyout(myproc()->pagetable,(uint64)raddr,(char *)&mask,sizeof(mask));
  sfence_vma();
  return 0;
}
#endif

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
