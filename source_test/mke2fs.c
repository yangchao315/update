#include "common.h"

int pox_system(const char *cmd_line)
{
    int ret = 0;
    __sighandler_t old_handler;

    old_handler = signal(SIGCHLD, SIG_DFL);
    ret = system(cmd_line);
    signal(SIGCHLD, old_handler);
    return ret;
}

int main()
{
int ret=0;
ret=pox_system("mke2fs -T ext4 -L root2 /dev/mmcblk0p3");
printf("ret is %x\n",ret);
return 0;
}
