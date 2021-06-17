#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char* argv[])
{
    int lg = atoi(argv[1]);
    sleep(lg);
    exit();
}