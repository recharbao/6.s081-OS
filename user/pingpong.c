#include "kernel/types.h"
#include "user/user.h"

int main()
{
    int parent_fd[2];
    int child_fd[2];

    pipe(parent_fd);
    pipe(child_fd);

    char a = 1;
    if (fork() > 0)
    {
        write(parent_fd[1], &a, 1);
        close(parent_fd[1]);
        read(child_fd[0], &a, 1);
        close(child_fd[0]);
        wait();
        if(a == 0){
            printf("pong\n");
        }
    }else
    {
        
        read(parent_fd[0], &a, 1);
        close(parent_fd[0]);

        if (a == 1){
            printf("ping\n");
        }

        a = 0;

        write(child_fd[1], &a, 1);
    }
    
    exit();
}
