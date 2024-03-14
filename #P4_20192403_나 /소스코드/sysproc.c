#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
pte_t *walkpgdir(pde_t *pgdir, const void *va, int alloc);

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_getvp(void)
{
  return myproc()->sz / PGSIZE;  
}

int
sys_getpp(void) {
  struct proc *p = myproc();
  int res = 0;

  for(uint i=0; i<myproc()->sz; i+=PGSIZE) {
    pte_t *pte = walkpgdir(p->pgdir, (void *)i, 0);
    if(pte && (*pte & PTE_P))
      res++;
  }
  return res;
}

int
sys_ssualloc(void)
{
  int addr;  // 증가할 메모리 주소를 저장할 변수
  int n;     // 증가시킬 크기를 저장할 변수

  // 시스템 콜에서 전달된 인자로부터 크기를 얻어옴
  if (argint(0, &n) < 0 || n <= 0 || n%PGSIZE != 0) {
    return -1; // 유효하지 않은 값을 입력받았을 때 -1을 반환
  }

  // 현재 프로세스의 주소 반환
  addr = myproc()->sz;

  // growproc 함수를 호출하여 프로세스의 크기를 조절
  myproc()->sz+=n;

  // 증가된 메모리의 시작 주소를 반환
  return addr;
}
