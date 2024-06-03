#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
// 尤其要注意,每次fork的时候,管道实际上也fork了,需要在子程序和父程序中都关掉
int main()
{
    int p[1][1];
    pipe(p[0]);
    for (int i = 2; i <= 35; i++)
    {
        write(p[0][1], &i, 4);
    }
    close(p[0][1]);
    int len = 4;
    int cnt = 0;
    while (1)
    {
        int prime;
        len = read(p[0][0], &prime, 4);
        if (len != 4)
        {
            break;
        }
        else
        {
            printf("prime %d\n", prime);
            pipe(p[1]);
            int pid = fork();
            if (pid == 0)
            {
                close(p[1][0]);
                int temp = 0;
                while (1)
                {
                    len = read(p[0][0], &temp, 4);
                    if (len != 4)
                    {
                        break;
                    }
                    else
                    {
                        if (temp % prime != 0)
                        {
                            write(p[1][1], &temp, 4);
                        }
                    }
                }
                close(p[0][0]);
                close(p[1][1]);
                exit(0);
            }
            else
            {
                ++cnt;
                close(p[0][0]);
                p[0][0] = p[1][0];
                p[0][1] = p[1][1];
                close(p[0][1]);
            }
        }
    }
    for (int i = 0; i < cnt; i++)
    {
        wait(0);
    }
    close(p[0][0]);
    exit(0);
}