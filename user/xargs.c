#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(2, "xargs: no command provided\n");
        fprintf(2, "usage: xargs command\n");
        exit(1);
    }
    char arg[100];
    char *argv_next[argc+1];
    for (int i = 1; i < argc; i++)
    {
        argv_next[i - 1] = argv[i];
    }
    argv_next[argc - 1] = arg;
    argv_next[argc] = 0;
    int cnt = 0;
    while (1)
    {
        do
        {
            int len = read(0, &arg[cnt], 1);
            if(len == 0)
            {
                goto end;
            }
            if(arg[cnt] == '\n')
            {
                arg[cnt] = 0;
                break;
            }
            ++cnt;
        }while(1);
        cnt=0;
        int pid = fork();
        if(pid == 0)
        {
            exec(argv[1], argv_next);
            printf("xargs: exec %s failed\n", argv[1]);
            exit(1);
        }
        else
        {
            wait(0);
        }
    }

    end:
    exit(0);
}