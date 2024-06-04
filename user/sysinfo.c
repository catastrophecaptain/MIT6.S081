#include "kernel/types.h"
#include "kernel/sysinfo.h"
#include "user/user.h"
int main(void)
{
    struct sysinfo info;
    sysinfo(&info);
    printf("Free memory: %d\n", info.freemem);
    printf("Number of processes: %d\n", info.nproc);
    exit(0);
}