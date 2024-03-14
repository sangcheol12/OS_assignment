#include "types.h"
#include "stat.h"
#include "user.h"

#define PNUM 4 // 프로세스 개수

void scheduler_func(void)
{
    int pid, i;
    
    printf(1,"start of scheduler\n");
    for(i = 0; i < PNUM; i++){
        if((pid = fork()) == 0) {// 프로세스 생성
            int pidChild = getpid();
            printf(1,"set_sche_info() pid = %d\n", pidChild);
            set_sche_info((i+1)*(i+1),130);
            while(1);
        }
        else if(pid < 0){
            printf(1, "fork error\n");
        }
    }
    for(int i=0;i<PNUM;i++)
        wait();
    printf(1,"end of scheduler_test\n");
    return;
}

int main(void)
{
    scheduler_func();
    destroyRunQueue();
    exit();
}