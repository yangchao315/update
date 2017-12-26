#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

char *Kernel_File = "/sdcard/update/Kernel_Update.tar.bz2";
#define MOUNT_KERNEL_PATH   "mount /dev/nandblk0p2 /mnt/"

static void
check_and_fclose(FILE *fp, const char *name) {
    fflush(fp);
    if (ferror(fp)) printf("Error in %s\n(%s)\n", name, strerror(errno));
    fclose(fp);
}

static void
copy_file(const char* source, const char* destination) {
    char buf[4096];
    int bytes_in, bytes_out;
    int src_len = 0;
    int dst_len = 0;
    FILE *dst = fopen(destination, "wb");
    if (dst == NULL) {
        printf("Can't open %s\n", destination);
    } else {
        FILE *src = fopen(source, "rb");
        if (src != NULL) {
            fseek(src, 0, SEEK_SET);  // Since last write
	    while ((bytes_in = fread(buf, 1, 4096, src)) > 0 ) {
                src_len += bytes_in;
                bytes_out = fwrite (buf, 1, bytes_in, dst);
		dst_len += bytes_out;
            }
	    printf("Copy %d, Copied %d\n", src_len, dst_len);
            check_and_fclose(src, source);
        }
        check_and_fclose(dst, destination);
    }
}



int pox_system(const char *cmd_line)
{
    int ret = 0;
    //sighandler_t old_handler;

   // old_handler = signal(SIGCHLD, SIG_DFL);
    ret = system(cmd_line);
   // signal(SIGCHLD, old_handler);

    return ret;
}


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
