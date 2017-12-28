#include "common.h"

char *Kernel_File = "/sdcard/update/Kernel_Update.tar.bz2";
#define MOUNT_KERNEL_PATH   "mount /dev/nandblk0p2 /mnt/"

int WriteKernelFn()
{
	int ret=0;
	/*mount boot partition to /mnt*/
	ret=pox_system(MOUNT_KERNEL_PATH);
    if (!ret) {
        printf("[WriteKernelFn] Mount kernel path success!\n");
    }
    else {
        printf("[WriteKernelFn] Mount kernel path failed!\n");
    }
	/*access if kernel_file exist */
	if(access(Kernel_File, F_OK) == 0){
		char cmd_line[128];
		fprintf(stderr, "[WriteKernelFn]find %s in udisk!\n",Kernel_File);
		sprintf(cmd_line, "%s%s%s", "tar -jxvf ", Kernel_File," -C /tmp");
		pox_system(cmd_line);
		
	/*update kernel & dtb*/
		copy_file("/tmp/zImage","/mnt/zImage-v1");
		copy_file("/tmp/zImage","/mnt/zImage-v2");
		pox_system("sync");
		copy_file("/tmp/dtb","/mnt/dtb-v1");
		copy_file("/tmp/dtb","/mnt/dtb-v2");	
		pox_system("sync");
	}else{
		printf("[WriteKernelFn] no Kernel file !\n");
	}

	if(0==umount("/mnt")){
		printf("[WriteKernelFn]umount /mnt success!\n");
	}
	return 0;
}

int main(int argc,char **argv)
{	
	WriteKernelFn();
	return 0;
}
