#include "update.h"

#define RFLAG_ADDR		0x0027f000 /* 2.5MB-4096 */
#define RFLAG_START		0x5a
#define RFLAG_FINISH	0xa5
static void set_flag(const char* target_media,
		int offset, int flag)
{
	FILE* f = NULL;
	if (target_media == NULL)
		return;
	f = fopen(target_media, "wb");
	if (f == NULL) {
		fprintf(stderr, "[SetFlag]can't open %s: %s\n",
				target_media, strerror(errno));
		return;
	}
	if (!(fseek(f, offset, SEEK_SET))) {
		fprintf(stderr, "[SetFlag]write FLAG %#x at %#x\n", flag, offset);
		putc(flag, f);
	} else {
		fprintf(stderr, "[SetFlag]fseek to position: %#x failed\n", offset);
	}
	fflush(f);
	fsync(fileno(f));
	fclose(f);
	f = NULL;
}

static int get_flag(const char* target_media, int offset)
{
	FILE* f = NULL;
	int flag = 0;
	if (target_media == NULL)
		return 0;
	f = fopen(target_media, "rb");
	if (f == NULL) {
		fprintf(stderr, "[SetFlag]can't open %s: %s\n",
				target_media, strerror(errno));
		return 0;
	}
	if (!(fseek(f, offset, SEEK_SET))) {
		flag = getc(f);
		fprintf(stderr, "[GetFlag]read FLAG %#x at %#x\n", flag, offset);
	} else {
		fprintf(stderr, "[GetFlag]fseek to position: %#x failed\n", offset);
	}
	fclose(f);
	f = NULL;
	return flag;
}
int mount_device(char* location,const char* mount_point,char* fs_type)
{
	int status=0;
	/*check mount_point whether existed*/
	if(NULL==opendir(mount_point)){
		status=mkdir(mount_point,0755);
			if(-1==status){
				fprintf(stderr, "[mount_device]mkdir failed!\n ERROR in %s \n",strerror(errno));
				return status;
			}else{
				fprintf(stderr, "[mount_device]mkdir %s successed\n",mount_point);
			}
	}
	/*mount*/
	if (mount(location, mount_point, fs_type,
			MS_NOATIME | MS_NODEV | MS_NODIRATIME, "") < 0) {
		fprintf(stderr, "[mount_device]failed to mount %s at %s !\n",location,mount_point);
		return -1;
	}else{
		fprintf(stderr, "[mount_device]mount %s at %s successed\n",location,mount_point);
		source_media=location;
	    }
	return 0;
}

int update_bt(void){
	fprintf(stderr,"[recovery]in update_bt...\n");
	FILE*f_bt1,*f_bt2;	
	f_bt1=fopen(BT_MAC_BACKUP_PATH, "rb");
	f_bt2=fopen(BT_MAC_BACKUP_PATH2, "rb");
	
	if((f_bt1!=NULL)&&(f_bt2!=NULL)){
		fprintf(serial_fp,"[recovery]prepare to copy bt files ...\n");
		
		sprintf(user_part, "%sp2", target_media);//mount boot partition
		mkdir("/btmp1", 0755);
		if (mount(user_part, "/btmp1", "ext4", MS_NOATIME | MS_NODEV | MS_NODIRATIME, "") < 0) {
			fprintf(serial_fp, "[recovery]mount %s to /btmp1 failed!\n",user_part);
		} else{
			fprintf(serial_fp, "[recovery]mount %s to /btmp1 success!\n",user_part);
			copy_file(BT_MAC_BACKUP_PATH,"/btmp1/bt.inf");
			fprintf(serial_fp, "[recovery]copy bt.inf done\n");
			copy_file(BT_MAC_BACKUP_PATH2,"/btmp1/trail.inf");
			if(NULL!=fopen(BT_MAC_PATH3, "rb")){
				fprintf(serial_fp, "[recovery]copy trail.inf to %s done-----1111\n",user_part); 
				}
			}
	
		sprintf(user_part, "%sp3", target_media);//mount root partition
		mkdir("/btmp2", 0755);
		if (mount(user_part, "/btmp2", "ext4", MS_NOATIME | MS_NODEV | MS_NODIRATIME, "") < 0) {
			fprintf(serial_fp, "[recovery]mount %s to /btmp2 failed!\n",user_part);
		} else {
			fprintf(serial_fp, "[recovery]mount %s to /btmp2 success!\n",user_part);
			copy_file(BT_MAC_BACKUP_PATH2,"/btmp2/var/lib/trail/bt.inf");
			if(NULL!=fopen(BT_MAC_PATH2, "rb")){
				fprintf(serial_fp, "[recovery]copy trail.inf to %s done-----1111\n",user_part); 
				}
			mkdir("/btmp2/var/lib/csr_synergy",0755);
			copy_file(BT_MAC_BACKUP_PATH,"/btmp2/var/lib/csr_synergy/bt.inf");
			if(NULL!=fopen(BT_SYNERGY_PATH, "rb")){
				fprintf(serial_fp, "[recovery]copy bt.inf to %s done-----2222\n",user_part);
				}
			}
		fclose(f_bt1);
		fclose(f_bt2);
		umount("/btmp1");
		remove("/btmp1");
		umount("/btmp2");
		remove("/btmp2");
	}else{
		fprintf(serial_fp, "[recovery]%s or %s does not exist\n",BT_MAC_BACKUP_PATH, BT_MAC_BACKUP_PATH2);
		}
	return 0;
}

int find_update_zip(void)
{
	if (access(UPDATE_PACKAGE_PATH, F_OK) ==0) {
		fprintf(stderr, "[recovery]find update.zip in udisk!\n");
		is_factory_production = true;//factory-mode means recovery from udisk or extern SD card
	}else{
		fprintf(stderr, "[recovery]no update.zip in udisk!\n");	
		sync();
		return -1;
	}
	return 0;
}

char* WriteUbootFn(char* filename,char* partition) {
    char* result = NULL;
    long write_position;
    bool is_emmc = false;

    if (strcmp(partition, "/dev/mmcblk0") == 0) is_emmc = true;

    if (strlen(partition) == 0) {
        fprintf(stderr, "[WriteUbootFn] partition argument to %s can't be empty", partition);
        goto done;
    }
    if (strlen(filename) == 0) {
        fprintf(stderr, "[WriteUbootFn] file argument to %s can't be empty", filename);
        goto done;
    }
    bool success;
    fprintf(stderr, "[WriteUbootFn] open %s\n", filename);
    FILE* f = NULL;
    f = fopen(filename, "rb");
    if (f == NULL) {
        fprintf(stderr, "[WriteUbootFn]: can't open %s: %s\n",
                 filename, strerror(errno));
        result = strdup("");
        goto done;
    }
    long read_position;
    read_position = 0;
    if (!!(fseek(f, read_position, SEEK_SET))) {
        fprintf(stderr, "[WriteUbootFn]fseek failed, %s\n", strerror(errno));
        result = strdup("");
        goto done;
    }
    fprintf(stderr, "[WriteUbootFn]fseek to read position: %d\n", (int)read_position);
    fprintf(stderr, "[WriteUbootFn]open %s\n", partition);
    FILE* raw_f = NULL;
    raw_f = fopen(partition,"r");
    if (raw_f == NULL) {
        fprintf(stderr, "[WriteUbootFn]: can't open %s: %s\n",
               partition, strerror(errno));
        result = strdup("");
        goto done;
    }
    fclose(raw_f);
    raw_f = NULL;
    raw_f = fopen(partition,"wb");
    if (raw_f == NULL) {
        fprintf(stderr, "[WriteUbootFn]: can't open %s: %s\n",
                partition, strerror(errno));
        result = strdup("");
        goto done;
    }
    int fd;
    fd = open(partition, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "[WriteUbootFn]: can't open %s: %s\n",
                partition, strerror(errno));
        result = strdup("");
        goto done;
    }
    ioctl(fd, BLKSSZGET, &write_position);
    fprintf(stderr, "[WriteUbootFn]store uboot at offset %ld of %s\n",
        write_position, strerror(errno));
    close(fd);
    if (!!(fseek(raw_f, write_position, SEEK_SET))) {
        fprintf(stderr, "[WriteUbootFn]fseek failed, %s\n", strerror(errno));
        result = strdup("");
        goto done;
    }
    fprintf(stderr, "[WriteUbootFn]fseek to write position: %d\n", (int)write_position);
    success = true;
    char* buffer = malloc(BUFSIZ);
    if (buffer == NULL) {
        success = false;
        fprintf(stderr, "failed to alloc %d bytes", BUFSIZ);
        goto done_malloc;
    }
    int read;
    int wrote;
    int read_img_count = 0;
    int write_img_count = 0;
    while (success && (read = fread(buffer, 1, BUFSIZ, f)) > 0) {
        read_img_count += read;
        wrote = fwrite(buffer, 1, read, raw_f);
        write_img_count += wrote;
        success = success && (wrote == read);
        if (!success) {
            fprintf(stderr, "[WriteUbootFn]write to %s: %s\n",
                    partition, strerror(errno));
        }
    }
    fflush(raw_f);
    fsync(fileno(raw_f));

    /* Write the backup uboot from 1MB + 1 sector for EMMC
     * NANDDISK do the backup internal
     * */
    if (is_emmc) {
	write_position += 1024 * 1024;
        if (!!(fseek(raw_f, write_position, SEEK_SET))) {
            fprintf(stderr, "[WriteUbootFn]fseek failed, %s\n", strerror(errno));
            result = strdup("");
            goto done;
        }
        fprintf(stderr, "[WriteUbootFn]fseek to write position: %d\n", (int)write_position);
        success = true;
	memset(buffer, 0, BUFSIZ);
        read = 0;
	wrote = 0;
        read_img_count = 0;
        write_img_count = 0;
        while (success && (read = fread(buffer, 1, BUFSIZ, f)) > 0) {
            read_img_count += read;
            wrote = fwrite(buffer, 1, read, raw_f);
            write_img_count += wrote;
            success = success && (wrote == read);
            if (!success) {
                fprintf(stderr, "[WriteUbootFn]write to %s: %s\n",
                        partition, strerror(errno));
            }
        }
        fflush(raw_f);
        fsync(fileno(raw_f));
    }

    // write commit flag
    if (!!(fseek(raw_f, COMMIT_FLAG_ADDR, SEEK_SET))) {
        fprintf(stderr, "[WriteUbootFn]fseek failed, %s\n", strerror(errno));
        result = strdup("");
        goto done;
    }
    success = success && (1 == fwrite(buffer, 1, 1, raw_f));
    if (!success) {
        fprintf(stderr, "[WriteUbootFn]write to %s: %s\n",
                partition, strerror(errno));
    }

    free(buffer);

    /* Write uboot update flag(0x3c) at the 2.5MB-4096+1 byte.
     * Uboot will check it to know if uboot has been updated.
     */
    int flag_offset = 0x0027f000 + 1;
    int flag = 0x3c;
    if (!(fseek(raw_f, flag_offset, SEEK_SET))) {
	fprintf(stderr, "[WriteUbootFn]set Uboot update flag %#x at %#x\n",flag, flag_offset);
	putc(flag, raw_f);
    } else {
        fprintf(stderr, "[WriteUbootFn]fseek Uboot update flag offset failed, %s\n", strerror(errno));
    }
    fflush(raw_f);
    fsync(fileno(raw_f));
	fprintf(stderr, "[WriteUbootFn]fsync done! \n");
done_malloc:
    fclose(f);
    f = NULL;
    fclose(raw_f);
    raw_f = NULL;

    fprintf(stderr,"[WriteUbootFn]%s %s partition from %s\n",
           success ? "wrote" : "failed to write", partition, filename);

    result = success ? partition : strdup("");

done:
    if (result != partition) {
		free(partition);
		free(filename);
		}
	fprintf(stderr, "[WriteUbootFn]write u-boot.csr successed! \n");
    return result;
}

int WriteKernelFn()
{
	int ret=0;
	/*access whether kernel_file exist */
	if(access(Kernel_File, F_OK) == 0){
		char cmd_line[128];
		fprintf(stderr, "[WriteKernelFn]find %s in udisk!\n",Kernel_File);
		/*mount boot partition to /mnt*/
		ret=pox_system(MOUNT_KERNEL_PATH);
		if (!ret) {
        	printf("[WriteKernelFn] Mount kernel path success!\n");
    	}else {
        	printf("[WriteKernelFn] Mount kernel path failed!\n");
    	}
		sprintf(cmd_line, "%s%s%s", "tar -jxvf ", Kernel_File," -C /tmp");
		pox_system(cmd_line);
		
		/*update kernel & dtb & csrvisor.bin*/
		copy_file("/tmp/zImage","/mnt/zImage-v1");
		copy_file("/tmp/zImage","/mnt/zImage-v2");
		pox_system("sync");
		copy_file("/tmp/dtb","/mnt/dtb-v1");
		copy_file("/tmp/dtb","/mnt/dtb-v2");	
		pox_system("sync");
		copy_file("/tmp/csrvisor.bin","/mnt/csrvisor.bin");

		if(0==umount("/mnt")){
			printf("[WriteKernelFn]umount /mnt success!\n");
		}
	}else{
		printf("[WriteKernelFn] no Kernel file !\n");
		return -1;
	}
	return 0;
}


int WriteAppFn()
{
	int ret=0;
	int update_mode = 1;
	if(update_mode == 1){
		pox_system("umount /dev/nandblk0p3");
		 ret = pox_system(FORMAT_DISK_CMD);
		if (!ret) {
            printf("[WriteAppFn] Format disk success!\n");
        }
        else {
            printf("[WriteAppFn] Format disk failed!\n");
            return 1;
        }
	}
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
		fprintf(stderr, "[WriteAppFn] find %s in udisk!\n",App_File);
		sprintf(cmd_line, "%s%s%s", "tar -jxf ", App_File," -C /mnt");
		pox_system(cmd_line);
		pox_system("sync");
	}else{
		printf("[WriteAppFn] no App file !\n");
	}

	if(0==umount("/mnt")){
		printf("[WriteAppFn] umount /mnt success!\n");
	}
	return 0;
}


int	main(int argc, char **argv) {
	const char *send_intent = NULL;
    const char *update_package = NULL;
    int  wipe_cache = 0;
    int status = INSTALL_SUCCESS;
	
	/*for recovery LOG*/
    mkdir("/sdcard/cache", 755);
	pox_system("mount -o remount rw /");
	//mkdir(mount_point, 755);//create /sdcard as the mount point,maybe we will use /mnt 
	serial_fp = fopen("/dev/ttySiRF1", "w");
    if (serial_fp != NULL) fprintf(serial_fp, "[recovery]Open ttySiRF1 success!\n");
	/*redirect stdout`stderr*/
    freopen(TEMPORARY_LOG_FILE, "a", stdout); setbuf(stdout, NULL);
    freopen(TEMPORARY_LOG_FILE, "a", stderr); setbuf(stderr, NULL);

	/*find target media*/   
   	if (access(nand_device, F_OK) == 0){
    /* boot media is nand, and it will be updated*/
        target_media = nand_device;
	}else{
    /* boot media is emmc, and it will be updated*/
		target_media = mmc_device;
	}
    user_partno = 7;//maybe not
    sprintf(user_part_nand, "/dev/nandblk0p%d", user_partno);
    sprintf(user_part_sd, "/dev/mmcblk0p%d", user_partno);
    usleep(1000);
	
    if (access(udisk, F_OK) ==0) {
		fprintf(serial_fp, "[recovery]udisk exists\n");
		/*mount udisk*/
		mount_device(udisk,mount_point,"vfat");
		/*find update.zip*/
		if(-1==find_update_zip()){
			fprintf(serial_fp, "[recovery]can`t find update.zip\n");
			umount(mount_point);
		}	
        fprintf(serial_fp, "[recovery]Recovery from %s to %s\n",source_media, target_media);	
    }else{
		fprintf(serial_fp, "[recovery]can`t find udisk !\n");
	}

	/*set recovery start flag 0x5a*/
    set_flag(target_media, RFLAG_ADDR, RFLAG_START);
		fprintf(serial_fp, "\n[recovery]Set recovery start flag, read=%#x\n",get_flag(target_media, RFLAG_ADDR));

	//WriteUbootFn(Uboot_Name,Uboot_Path);
	WriteKernelFn();
	//WriteAppFn();
	/*update update.zip*/
	if(status == INSTALL_SUCCESS) {
            sprintf(user_part, "%sp7", target_media);
	    mkdir("/user", 0755);
            if (mount(user_part, "/user", "vfat", MS_NOATIME | MS_NODEV | MS_NODIRATIME, "") < 0) {
		fprintf(serial_fp, "[recovery]mount %s to /user failed!\n",user_part);
	    } else {
		mkdir("/user/update",0755);
		copy_file(update_package, UPDATEZIP_PATH);
	        fprintf(serial_fp, "[recovery]copy update.zip done!\n");
	    }
		remove("/user");
	}
	/*update BT files*/
	if(status == INSTALL_SUCCESS){
		fprintf(serial_fp, "[recovery]updating bt!\n");
		update_bt();
	}

	finish_recovery(send_intent);
	
	if(status == INSTALL_SUCCESS){
	/*set recovery finish flag 0xa5*/
   set_flag(target_media,RFLAG_ADDR,RFLAG_FINISH);
   		fprintf(serial_fp, "\n[recovery]Set recovery finish flag, read=%#x\n",get_flag(target_media, RFLAG_ADDR));
	}


	if (status == INSTALL_SUCCESS){
		fprintf(serial_fp, "[recovery]recovery finished\n");
	}else{ 	
		fprintf(serial_fp, "[recovery]recovery failed\n");
        fprintf(serial_fp, "[recovery]For more log information, please check %s now\n", TEMPORARY_LOG_FILE);    	 
		}
	if(0==umount("/sdcard")){
		remove("/sdcard");
		fprintf(serial_fp, "[recovery]umount /sdcard done...\n");
	}
    return EXIT_SUCCESS;
}
