#include "update.h"

#define RFLAG_ADDR		0x0027f000 /* 2.5MB-4096 */
#define RFLAG_START		0x5a
#define RFLAG_FINISH	0xa5
static void set_flag(const char* target_media,int offset, int flag)
{
	FILE* f = NULL;
	if (target_media == NULL)
		return;
	f = fopen(target_media, "wb");
	if (f == NULL) {
		fprintf(stderr, "[set_flag] Can't open %s: %s\n",target_media, strerror(errno));
		return;
	}
	if (!(fseek(f, offset, SEEK_SET))) {
		fprintf(stderr, "[set_flag] Write FLAG %#x at %#x\n", flag, offset);
		putc(flag, f);
	} else {
		fprintf(stderr, "[set_flag] Fseek to position: %#x failed\n", offset);
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
		fprintf(stderr, "[get_flag] Can't open %s: %s\n",target_media, strerror(errno));
		return 0;
	}
	if (!(fseek(f, offset, SEEK_SET))) {
		flag = getc(f);
		fprintf(stderr, "[get_flag] Read FLAG %#x at %#x\n", flag, offset);
	} else {
		fprintf(stderr, "[get_flag] Fseek to position: %#x failed\n", offset);
	}
	fclose(f);
	f = NULL;
	return flag;
}
static int erase_flash(int fd, int offset, int bytes)
{
	int err;
	struct erase_info_user erase;
	erase.start = offset;
	erase.length = bytes;
	err = ioctl(fd,MEMERASE, &erase);
	if (err < 0) {
		fprintf(serial_fp,"[erase_flash] MEMERASE failed\n");
		return 1;
	}
	fprintf(serial_fp, "[erase_flash] Erased %d bytes from address 0x%.8x in flash\n", bytes, offset);
	return 0;
}

static int file_to_flash(int fd, int offset, int len, const char *filename)
{
	u_int8_t *buf = NULL;
	FILE *fp;
	int err;
	int size = len * sizeof(u_int8_t);
	int n = len;

	if (offset != lseek(fd, offset, SEEK_SET)) {
		fprintf(serial_fp,"[file_to_flash] lseek() failed!\n");
		return 1;
	}
	if ((fp = fopen(filename, "r")) == NULL) {
		fprintf(serial_fp,"[file_to_flash] fopen() failed!\n");
		return 1;
	}
retry:
	if ((buf = (u_int8_t *) malloc(size)) == NULL) {
		fprintf(serial_fp, "%s: malloc(%#x) failed\n", __func__, size);
		if (size != BUFSIZ) {
			size = BUFSIZ;
			fprintf(serial_fp, "%s: trying buffer size %#x\n", __func__, size);
			goto retry;
		}
		printf("malloc()");
		fclose(fp);
		return 1;
	}
	do {
		if (n <= size)
			size = n;
		if (fread(buf, size, 1, fp) != 1 || ferror(fp)) {
			fprintf(serial_fp, "%s: fread, size %#x, n %#x\n", __func__, size, n);
			printf("fread()");
			free(buf);
			fclose(fp);
			return 1;
		}
		err = write(fd, buf, size);
		if (err < 0) {
			fprintf(serial_fp, "%s: write, size %#x, n %#x\n", __func__, size, n);
			printf("write()");
			free(buf);
			fclose(fp);
			return 1;
		}
		n -= size;
	} while (n > 0);

	if (buf != NULL)
		free(buf);
	fclose(fp);
	fprintf(serial_fp,"Copied %d bytes from %s to address 0x%.8x in flash\n", len, filename, offset);
	return 0;
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
char* WriteM3Fn(char* filename,char* partition) {
	char* result = NULL;
	int rfd, wfd;
	int rf_size = 0, err = 0;
	struct stat st;
	bool success = true;

    if (strlen(partition) == 0) {
        fprintf(serial_fp, "[WriteM3Fn] partition argument to %s can't be empty", partition);
        goto done;
    }
    if (strlen(filename) == 0) {
        fprintf(serial_fp, "[WriteM3Fn] file argument to %s can't be empty", filename);
        goto done;
    }

	if ((rfd = open(filename, O_SYNC | O_RDONLY)) < 0) {
		fprintf(serial_fp, "[WriteM3Fn] open %s failed\n",filename);
        	result = strdup("");
		goto done;
	}

	if ((wfd = open(partition, O_SYNC | O_RDWR)) < 0) {
		fprintf(serial_fp, "[WriteM3Fn] open %s failed\n",partition);
        	result = strdup("");
		goto done;
	}

	stat(filename, &st);
	rf_size = st.st_size;

	fprintf(stderr, "[WriteM3Fn] erase flash...\n");
	err = erase_flash(wfd, 0, rf_size);
	if (err) {
		fprintf(serial_fp, "[WriteM3Fn] erase SPI NOR failed\n");
		success = false;
		goto done;
	}
	fprintf(serial_fp, "[WriteM3Fn]%s (%d bytes) be worte\n", filename, rf_size);
	err = file_to_flash(wfd, 0, rf_size, filename);
	if (err) {
		fprintf(serial_fp, "[WriteM3Fn] wirte Freertos to SPI NOR failed\n");
		success = false;
		goto done;
	}

	/* close device */
	if (close(rfd) < 0)
		fprintf(serial_fp, "[WriteM3Fn] close rfd failed\n");
	if (close(wfd) < 0)
		fprintf(serial_fp, "[WriteM3Fn] close wfd failed\n");

	result = success ? partition : strdup("");

done:
    if (result != partition) {
		free(partition);
		free(filename);
		}
	fprintf(serial_fp, "[WriteM3Fn] write M3 successed! \n");
    return result;
}

int WriteKernelFn()
{
	int ret=0;
	int flag=0;
	flag=get_flag(target_media, RFLAG_ADDR);
	if(flag==RFLAG_START){
		/*5a-->system-A boot,update system-B*/
		/*mount boot2 partition to /mnt*/
		ret=pox_system(MOUNT_KERNEL_PATH2);
		if (!ret) {
				fprintf(serial_fp,"[WriteKernelFn] Mount kerne2 path success!\n");
				}else {
				fprintf(serial_fp,"[WriteKernelFn] Mount kerne2 path failed!\n");
				goto mount_err;
			}
	}else{
		/*a5-->system-B boot,update system-A*/
		/*mount boot1 partition to /mnt*/
		ret=pox_system(MOUNT_KERNEL_PATH);
		if (!ret) {
				fprintf(serial_fp,"[WriteKernelFn] Mount kerne1 path success!\n");
				}else {
				fprintf(serial_fp,"[WriteKernelFn] Mount kerne1 path failed!\n");
				goto mount_err;
		}
	}
	/*access whether kernel_file exist */
	if(access(Kernel_File, F_OK) == 0){
		char cmd_line[128];
		fprintf(serial_fp, "[WriteKernelFn] find %s in udisk!\n",Kernel_File);
		mkdir("/kernel_tmp", 755);
		sprintf(cmd_line, "%s%s%s", "tar -jxvf ", Kernel_File," -C /kernel_tmp");
		pox_system(cmd_line);
		
	/*update kernel & dtb*/
		copy_file("/kernel_tmp/zImage","/mnt/zImage-v1");
		copy_file("/kernel_tmp/zImage","/mnt/zImage-v2");
		pox_system("sync");
		copy_file("/kernel_tmp/dtb","/mnt/dtb-v1");
		copy_file("/kernel_tmp/dtb","/mnt/dtb-v2");
		pox_system("sync");
		copy_file("/kernel_tmp/csrvisor.bin","/mnt/csrvisor.bin");
		fprintf(serial_fp,"[WriteKernelFn] update Kernel success !\n");
		pox_system("rm -rf /kernel_tmp");
	}else{
		fprintf(serial_fp,"[WriteKernelFn] no Kernel file !\n");
	}
mount_err:
	if(0==umount("/mnt")){
		fprintf(serial_fp,"[WriteKernelFn] umount /mnt success!\n");
	}
	return 0;
}


int WriteAppFn()
{
	int ret=0;
	int flag=0;
	flag=get_flag(target_media, RFLAG_ADDR);
	if(flag==RFLAG_START){
		/*5a-->system-A boot,update system-B*/
		/*mount root2 partition to /mnt*/
		ret=pox_system(MOUNT_APP_PATH2);
			if (!ret) {
				fprintf(serial_fp,"[WriteAppFn] Mount root2 path success!\n");
			}else {
				fprintf(serial_fp,"[WriteAppFn] Mount root2 path failed!\n");
				goto mount_err;
			}
	}else{
		/*a5-->system-B boot,update system-A*/
		/*mount root partition to /mnt*/
		ret=pox_system(MOUNT_APP_PATH);
			if (!ret) {
				fprintf(serial_fp,"[WriteAppFn] Mount root path success!\n");
			}else {
				fprintf(serial_fp,"[WriteAppFn] Mount root path failed!\n");
				goto mount_err;
			}
	}
	/*access if App_File exist */
	if(access(App_File, F_OK) == 0){
		char cmd_line[128];
		fprintf(serial_fp, "[WriteAppFn] find %s in udisk!\n",App_File);
		sprintf(cmd_line, "%s%s%s", "tar -jxf ", App_File," -C /mnt");
		pox_system(cmd_line);
		pox_system("sync");
		fprintf(serial_fp,"[WriteAppFn] update App success !\n");
	}else{
		fprintf(serial_fp,"[WriteAppFn] no App file !\n");
	}

mount_err:
	if(0==umount("/mnt")){
		fprintf(serial_fp,"[WriteAppFn] umount /mnt success!\n");
	}
	return 0;
}


int WriteRootfsFn()
{
	int ret=0;
	int flag=0;
	flag=get_flag(target_media, RFLAG_ADDR);
	if(flag==RFLAG_START){
		/*0x5a-->system-A boot,update system-B*/
		/*mount root2 partition to /mnt*/
		ret=pox_system(FORMAT_ROOT2);
		fprintf(serial_fp,"[WriteRootfsFn] Format ret is %d \n",ret);
		if(!ret){
			fprintf(serial_fp,"[WriteRootfsFn] Format root2 success!\n");
		}else{
			fprintf(serial_fp,"[WriteRootfsFn] Format root2 failed!\n");
		}

		ret=pox_system(MOUNT_APP_PATH2);
		if (!ret) {
			fprintf(serial_fp,"[WriteRootfsFn] Mount root2 path success!\n");
		}else {
			fprintf(serial_fp,"[WriteRootfsFn] Mount root2 path failed!\n");
			goto mount_err;
		}

	}else{
		/*0xa5-->system-B boot,update system-A*/
		/*mount root partition to /mnt*/
		ret=pox_system(FORMAT_ROOT);
		if(!ret){
			fprintf(serial_fp,"[WriteRootfsFn] Format root success!\n");
		}else{
			fprintf(serial_fp,"[WriteRootfsFn] Format root failed!\n");
		}

		ret=pox_system(MOUNT_APP_PATH);
		if (!ret) {
			fprintf(serial_fp,"[WriteRootfsFn] Mount root path success!\n");				
		}else {
			fprintf(serial_fp,"[WriteRootfsFn] Mount root path failed!\n");
			goto mount_err;
		}
	}
	/*access if Rootfs_File exist */
	if(access(Rootfs_File, F_OK) == 0){
		char cmd_line[128];
		fprintf(serial_fp, "[WriteRootfsFn] find %s in udisk!\n",Rootfs_File);
		mkdir("/rootfs_tmp", 755);
		sprintf(cmd_line, "%s%s%s", "tar -xf ", Rootfs_File," -C /rootfs_tmp");
		pox_system(cmd_line);
		pox_system("cp /rootfs_tmp/rootfs/* -rf /mnt");
		pox_system("sync");
		pox_system("rm -rf /rootfs_tmp");
		fprintf(serial_fp,"[WriteRootfsFn] update rootfs success!\n");
	}else{
		fprintf(serial_fp,"[WriteRootfsFn] no Rootfs file !\n");
	}

mount_err:
	if(0==umount("/mnt")){
		fprintf(serial_fp,"[WriteRootfsFn] umount /mnt success!\n");
	}
	return 0;
}

int caculate_partition(void)
{
	char DEVICE_NAME[32];
	int num=0,i=0;
	for(i=0;i<10;i++)
	{
		sprintf(DEVICE_NAME,"%s%s%d",target_media,"p",i);
		if(!access(DEVICE_NAME,F_OK))
		{	
			num=i;	
		}
	}	
	fprintf(serial_fp, "[caculate_partition] get the last partition num is %d!\n",num);
	return num;
}

int	main(int argc, char **argv) {
	const char *send_intent = NULL;
    const char *update_package = NULL;
    int  wipe_cache = 0;
    int status = INSTALL_SUCCESS;
	int flag=0;
	
	pox_system("mount -o remount rw /");
    mkdir("/sdcard/cache", 755);
	//mkdir(mount_point, 755);//create /sdcard as the mount point,maybe we will use /mnt 
	serial_fp = fopen("/dev/ttySiRF1", "w");
    if (serial_fp != NULL) fprintf(serial_fp, "\n[recovery]Open ttySiRF1 success!\n");
	/*redirect stdout`stderr*/
    freopen(TEMPORARY_LOG_FILE, "a", stdout); setbuf(stdout, NULL);
    freopen(TEMPORARY_LOG_FILE, "a", stderr); setbuf(stderr, NULL);

	/*check target media*/   
   	if (access(nand_device, F_OK) == 0){
        target_media = nand_device;
	}else{
		target_media = mmc_device;
	}
	fprintf(serial_fp, "[recovery]Target_media is %s!\n",target_media);
    
    usleep(1000);
	
    if (access(udisk, F_OK) ==0) {
		fprintf(serial_fp, "[recovery]udisk exists\n");
		/*mount udisk*/
		mount_device(udisk,mount_point,"vfat");
		/*find update.zip*/
		if(-1==find_update_zip()){
			fprintf(serial_fp, "[recovery]can`t find update.zip\n");
			umount(mount_point);
			return EXIT_FAILURE;
		}	
        fprintf(serial_fp, "[recovery]Recovery from %s to %s\n",source_media, target_media);	
    }else{
		fprintf(serial_fp, "[recovery]can`t find udisk !\n");
		return EXIT_FAILURE;
	}

	//WriteUbootFn(Uboot_Name,Uboot_Path);
	//WriteM3Fn(M3_File,M3_Path);
	WriteKernelFn();
	WriteAppFn();
	if (access(Rootfs_File, F_OK) ==0) {
		WriteRootfsFn();
		}
	/*update update.zip*/
	/*
	if(status == INSTALL_SUCCESS) {
		user_partno = caculate_partition();
		sprintf(user_part, "%sp%d", target_media,user_partno);
	    mkdir("/user", 0755);
            if (mount(user_part, "/user", "vfat", MS_NOATIME | MS_NODEV | MS_NODIRATIME, "") < 0) {
		fprintf(serial_fp, "[recovery]mount %s to /user failed!\n",user_part);
	    } else {
		mkdir("/user/update",0755);
		copy_file(update_package, UPDATEZIP_PATH);
	        fprintf(serial_fp, "[recovery]copy update.zip done!\n");
	    }
		remove("/user");
	}*/
	/*update BT files*/
	if(status == INSTALL_SUCCESS){
		fprintf(serial_fp, "[recovery]updating bt!\n");
		update_bt();
	}

	finish_recovery(send_intent);

	if (status == INSTALL_SUCCESS){
		flag=get_flag(target_media, RFLAG_ADDR);
		if(flag==RFLAG_START){
		/*0x5a-->system-A boot,so we are updating system-B
		update flag=0xa5,next time we boot System-B*/
			set_flag(target_media,RFLAG_ADDR,RFLAG_FINISH);
			fprintf(serial_fp, "\n[recovery]System-B update done!Set flag=%#x\n",get_flag(target_media, RFLAG_ADDR));
		}else{
		/*0xa5-->system-B boot,so we are updating system-A
		update flag=0x5a,next time we boot System-A*/
			set_flag(target_media,RFLAG_ADDR,RFLAG_START);
			fprintf(serial_fp, "\n[recovery]System-A update done!Set flag=%#x\n",get_flag(target_media, RFLAG_ADDR));
		}
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
	//pox_system("reboot");
    return EXIT_SUCCESS;
}
