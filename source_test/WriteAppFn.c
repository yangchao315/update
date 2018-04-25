#include "common.h"

//typedef void (*sighandler_t)(int);

char *App_File = "/sdcard/update/APP_Update.tar.bz2";
#define MOUNT_APP_PATH   "mount /dev/nandblk0p3 /mnt/"


int WriteAppFn()
{
	int ret=0;
	/*mount root partition to /mnt*/
	ret=pox_system(MOUNT_APP_PATH);
    if (!ret) {
        printf("[WriteAppFn] Mount root path success!\n");
    }
    else {
        printf("[WriteAppFn] Mount root path failed!\n");
    }
	/*access if App_File exist */
	if(access(App_File, F_OK) == 0){
		char cmd_line[128];
		fprintf(stderr, "[WriteAppFn]find %s in udisk!\n",App_File);
		sprintf(cmd_line, "%s%s%s", "tar -jxf ", App_File," -C /mnt");
		pox_system(cmd_line);
		
	/*update kernel & dtb*/	
		pox_system("sync");
	}else{
		printf("[WriteAppFn] no Kernel file !\n");
	}

	if(0==umount("/mnt")){
		printf("[WriteAppFn]umount /mnt success!\n");
	}
	return 0;
}

int main(int argc,char **argv)
{	
	WriteAppFn();
	return 0;
}
