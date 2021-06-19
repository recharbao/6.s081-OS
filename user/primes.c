#include "kernel/types.h"
#include "user/user.h"

int main()
{
    int *in_fd;
    int *out_fd;

    int r[2];
    int w[2];

    pipe(r);
    in_fd = r;

    if (fork() > 0)
    {
        close(in_fd[0]);
        for(int i = 2; i <= 35; i++)
        {
            write(in_fd[1], &i, 4);
        }
        close(in_fd[1]);
        wait();
    }else
    {
        int a, base = 222; //n = 3;
        int *tmp;
        out_fd = w;

        // while (n--)
        while(1)
        {
            pipe(out_fd);
            
            close(in_fd[1]);
            if(read(in_fd[0], &base, 4))
            {
                printf("prime: %d\n", base);
            }else
            {
                close(in_fd[0]);
                break;
            }
            
            if (fork() > 0)
            {
                close(out_fd[0]);
                while (read(in_fd[0], &a, 4) > 0)
                {  
                    if (a % base != 0)
                        write(out_fd[1], &a, 4);
                }

                close(in_fd[0]);
                close(out_fd[1]);
                wait();
                break;
            }else
            {
                close(in_fd[0]);
                tmp = in_fd;
                in_fd = out_fd;
                out_fd = tmp;
            }
        }
    }
    
    exit();   
}

































// #include "kernel/types.h"
// #include "user/user.h"

// int main()
// {
//     int in_fd[2];
//     int out_fd[2];

//     pipe(out_fd);

//     if (fork() > 0)
//     {
//         // printf("prime !");
//         close(out_fd[0]);
//         for(int i = 2; i <= 35; i++)
//         {
//             write(out_fd[1], &i, 4);
//         }
//         close(out_fd[1]);
//         wait();
//     }else
//     {
//         int a, c, base = 222, n = 3;
        
//         // int a, n = 3;
//         while (n--)
//         {
//             pipe(in_fd);
//             // printf("dsa\n");
            
//             if (fork() > 0)
//             {
//                 printf("n = %d\n", n);
//                 close(out_fd[1]);
//                 close(in_fd[0]);
//                 // close(in_fd[1]);
//                 while (read(out_fd[0], &a, 4) > 0)
//                 {  
//                     printf("a = %d\n", a);
//                     write(in_fd[1], &a, 4);
//                 }

//                 printf("dsadas\n");
//                 close(out_fd[0]);
//                 printf("child process !\n");
//                 close(in_fd[1]);
//                 wait();
//                 break;
//             }else
//             {
//                 // sleep(10);
//                 printf("here1\n");
//                 close(in_fd[1]);
//                 // sleep(100);
//                 printf("here3\n");
//                 read(in_fd[0], &base, 4);
//                 printf("base = %d\n", base);

//                 close(out_fd[0]);
//                 while (read(in_fd[0], &c, 4) > 0)
//                 {
//                     printf("child = %d\n", c);
//                     if(a % base != 0)
//                     {
//                         write(out_fd[1], &a, 4);
//                         // printf("dsasdas=  %d\n", s);
//                     }       
//                 }

//                 close(out_fd[1]);
//                 close(in_fd[0]);

//                 printf("end !\n");
//             }
//         }
//     }
    
//     exit();   
// }