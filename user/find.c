#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
int getlsinfo(int len, char filename[len])
{
    // printf("3\n");
    int type = -1;
    int cnt = 0;
    int empty = 1;
    char type_src[10];
    while (1)
    {
        // printf("4");
        int read_len = read(0, filename + cnt, 1);
        if (read_len == 0)
        {
            return -1;
        }
        if (filename[cnt] == ' ')
        {
            filename[cnt] = 0;
            break;
        }
        ++cnt;
        if (cnt >= len)
        {
            return -2;
        }
    }
    cnt = 0;
    while (1)
    {
        read(0, type_src + cnt, 1);
        if (type_src[cnt] == ' ')
        {
            if (empty == 1)
            {
                continue;
            }
            else
            {
                type_src[cnt] = 0;
                break;
            }
        }
        else
        {
            ++cnt;
            empty = 0;
        }
        if (cnt >= sizeof(type_src))
        {
            return -2;
        }
    }
    type=atoi(type_src);
    while (1)
    {
        int read_len = read(0, type_src, 1);
        if (read_len == 0)
        {
            break;
        }
        if (type_src[0] == '\n')
        {
            break;
        }
    }

    // printf("filename: %s\n", filename);
    return type;
}
int main(int argc, char *argv[])
{
    char path[512];
    char file[512];
    char buf[512];
    int p[1];
    int pid = 0;
    if (argc < 2)
    {
        fprintf(2, "usage: find [path] file\n");
        exit(1);
    }
    else if (argc == 2)
    {
        strcpy(path, ".");
        strcpy(file, argv[1]);
    }
    else if (argc == 3)
    {
        strcpy(path, argv[1]);
        strcpy(file, argv[2]);
    }
    pipe(p);
    pid = fork();

    if (pid == 0)
    {
        close(p[0]);
        close(1);
        dup(p[1]);
        close(p[1]);
        char *argv_1[3];
        argv_1[0] = "ls";
        argv_1[1] = path;
        argv_1[2] = 0;
        exec("ls", argv_1);
        fprintf(2, "find: exec ls failed\n");
        exit(1);
    }
    // printf("1");
    close(p[1]);
    close(0);
    dup(p[0]);
    close(p[0]);
    while (1)
    {
        // printf("2");
        int type = getlsinfo(sizeof(buf), buf);
        // printf("3");
        if (strlen(buf) + strlen(path) + 1 >= sizeof(path))
        {
            type = -2;
        }
        switch (type)
        {
        case -1:
            // printf("3");
            goto out;
        case -2:
            fprintf(2, "find: file info too long\n");
            break;
        case T_DEVICE:
            break;
        case T_FILE:
            if (strcmp(buf, file) == 0)
            {
                printf("%s/%s\n", path, file);
            }
            break;
        case T_DIR:
            if (strcmp(buf, ".") == 0 || strcmp(buf, "..") == 0)
            {
                break;
            }
            pid = fork();
            if (pid == 0)
            {
                char *argv[4];
                argv[0] = "find";
                argv[1] = path;
                argv[2] = file;
                argv[3] = 0;
                strcpy(path + strlen(path), "/");
                strcpy(path + strlen(path), buf);
                exec("find", argv);
                fprintf(2, "find: exec find failed\n");
                exit(1);
            }
            wait(0);
            break;
        }
    }
out:
    wait(0);
    exit(0);
}