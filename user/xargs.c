#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

int main(int argc, char* argv[])
{
    // for(int i = 0; i < argc; i++){
    //     printf("i = %d : %s\n", i, argv[i]);
    // }

    // char buf[10];
    
    // gets(buf, 10);

    // printf("%s\n", buf);

    // exit();

    char path[64];
    path[0] = '.';
    path[1] = '/';
    strcpy(path+2, argv[1]);

    // printf("path = %s\n", path);

    char* param[64];

    for (int i = 1; i < argc; i++) {
        param[i - 1] = argv[i];
    }

    int s = argc - 1;
    char buf[64] = {0};
    char *p = buf;
    int len;
    
    while (gets(buf, sizeof(buf))){
        // printf("length = %d\n", strlen(buf));
        if ((len = strlen(buf)) < 1){
            break;
        }
        buf[len - 1] = 0;
        p = buf;
        s = argc - 1;

        // printf("buf = %s\n", buf);
        
        while(*p){
            // char tmp[64];
            // int i = 0;
            while (*p == ' '){p++;}
            param[s++] = p;
            while (*p != ' ' && *p != 0){
                p++;
            }
            *p++ = 0;
            // tmp[i] = 0;
        }
    
        param[s] = 0;

        // printf("path = %s\n", path);
        // printf("param_0 = %s\n", param[0]);
        // printf("param_1 = %s\n", param[1]);
        // printf("param_2 = %s\n", param[2]);

        if (s > MAXARG)
        {
            printf("argument too much !");
        }
        
        if (s < 1)
        {
            printf("argument not enough !");
        }
        
        if (fork() > 0)
        {
            wait();
        }else
        {
            exec(path, param);
            exit();
        }
    }
    exit();

}

