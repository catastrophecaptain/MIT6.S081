#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
int main()
{
    int p[2][2];
    pipe(p[0]);
    pipe(p[1]);
    char buf[2]="p";
    int pid=fork();
    if(pid==0)
    {
        read(p[0][0],buf,1);
        printf("%d: received ping\n",getpid());
        close(p[0][0]);
        write(p[1][1],buf,1);
        close(p[1][1]);
    }else
    {
        write(p[0][1],buf,1);
        close(p[0][1]);
        wait(0);
        read(p[1][0],buf,1);
        printf("%d: received pong\n",getpid());
        close(p[1][0]);
    }
    exit(0);
}