#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;
uint flowticks = 0;
extern void reSchedule();

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed)
      exit();
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      if(myproc() && myproc()->state == RUNNING) {
        myproc()->proc_tick++;
        myproc()->cpu_used++;
        myproc()->priority_tick++;
        flowticks++;
      }
      //if(myproc()->proc_tick >= myproc()->limit_tick)
        //myproc()->killed = 1;
      wakeup(&ticks);
      release(&tickslock);
    }
    #ifdef DEBUG
    if(myproc() !=0 && (tf->cs &3)==3) {
      if(myproc()->cpu_used >= myproc()->limit_tick && myproc()->pid>2) {
        cprintf("PID : %d, PRIORITY : %d, PROC_TICK: %d ticks, total_cpu_usage: %d ticks (%d)\n", myproc()->pid, myproc()->priority, myproc()->proc_tick, myproc()->cpu_used, 3);
        cprintf("PID: %d, terminated \n", myproc()->pid);
        //KILL PROCESS
        kill(myproc() -> pid);
      }
    }
    #else
    if(myproc()!=0 && myproc()->cpu_used == myproc()->limit_tick) myproc()->killed = 1;
    #endif
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;

  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  /*if(myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0+IRQ_TIMER && myproc()->proc_tick >= 30)
    yield();*/
  
  if(flowticks!=0 && flowticks%60 == 0) {
    //cprintf("Rescheduling\n");
    reSchedule();
  }
  
  if(myproc() && myproc()->state == RUNNING &&
    tf->trapno == T_IRQ0+IRQ_TIMER) {
    #ifdef DEBUG
    if(myproc()->cpu_used % 30 == 0 && myproc()->cpu_used > 0) {
      //30ticks print. 
      cprintf("PID : %d, PRIORITY : %d, PROC_TICK: %d ticks, total_cpu_usage: %d ticks (%d)\n", myproc()->pid, myproc()->priority, myproc()->proc_tick, myproc()->cpu_used, 1);
      myproc()->proc_tick = 0;
      yield();
    }
    #else
      yield();
    #endif
  }


  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}
