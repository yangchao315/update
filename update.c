/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <stdbool.h>

#include "update.h"

#define LOGW(...) fprintf(stdout, "W:" __VA_ARGS__)
#define printf(...) fprintf(stdout, "I:" __VA_ARGS__)

#define false 0
#define true  1

static int really_install_package(const char *path, int* wipe_cache)
{
    printf("Update location: %s\n", path);
    /* Try to open the package*/
    
    /* Verify and install the contents of the package.
     */

   // return try_update_binary(path, &zip, wipe_cache);
   return	INSTALL_SUCCESS;
}

int install_package(const char* path, int* wipe_cache, const char* install_file)
{
    FILE* install_log = fopen_path(install_file, "w");
    if (install_log) {
        fputs(path, install_log);
        fputc('\n', install_log);
    } else {
        printf("failed to open last_install: %s\n", strerror(errno));
    }
    int result = really_install_package(path, wipe_cache);
    if (install_log) {
        fputc(result == INSTALL_SUCCESS ? '1' : '0', install_log);
        fputc('\n', install_log);
        fclose(install_log);
    }
    return result;
}


static const int MAX_ARG_LENGTH = 4096;
static const int MAX_ARGS = 100;

// open a given path, mounting partitions as necessary
FILE* fopen_path(const char *path, const char *mode) {
    FILE *fp = fopen(path, mode);
    return fp;
}

// close a file, log an error if the error indicator is set
static void
check_and_fclose(FILE *fp, const char *name) {
    fflush(fp);
    if (ferror(fp)) printf("Error in %s\n(%s)\n", name, strerror(errno));
    fclose(fp);
}

// How much of the temp log we have copied to the copy in cache.
static long tmplog_offset = 0;

static void
copy_log_file(const char* source, const char* destination, int append) {
    FILE *log = fopen_path(destination, append ? "a" : "w");
    if (log == NULL) {
        printf("Can't open %s\n", destination);
    } else {
        FILE *tmplog = fopen(source, "r");
        if (tmplog != NULL) {
            if (append) {
                fseek(tmplog, tmplog_offset, SEEK_SET);  // Since last write
            }
            char buf[4096];
            while (fgets(buf, sizeof(buf), tmplog)) fputs(buf, log);
            if (append) {
                tmplog_offset = ftell(tmplog);
            }
            check_and_fclose(tmplog, source);
        }
        check_and_fclose(log, destination);
    }
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


// clear the recovery command and prepare to boot a (hopefully working) system,
// copy our log file to cache as well (for the system to read), and
// record any intent we were asked to communicate back to the system.
// this function is idempotent: call it as many times as you like.
static void
finish_recovery(const char *send_intent) {
    // By this point, we're ready to return to the main system...
    if (send_intent != NULL) {
        FILE *fp = fopen_path(INTENT_FILE, "w");
        if (fp == NULL) {
            printf("Can't open %s\n", INTENT_FILE);
        } else {
            fputs(send_intent, fp);
            check_and_fclose(fp, INTENT_FILE);
        }
    }
    // Copy logs to cache so the system can find out what happened.
    copy_log_file(TEMPORARY_LOG_FILE, LOG_FILE, true);
    copy_log_file(TEMPORARY_LOG_FILE, LAST_LOG_FILE, false);
    copy_log_file(TEMPORARY_INSTALL_FILE, LAST_INSTALL_FILE, false);
    chmod(LOG_FILE, 0600);
    //chown(LOG_FILE, 1000, 1000);   // system user
    chmod(LAST_LOG_FILE, 0640);
    chmod(LAST_INSTALL_FILE, 0644);
}

static int compare_string(const void* a, const void* b) {
    return strcmp(*(const char**)a, *(const char**)b);
}

static void
print_property(const char *key, const char *name, void *cookie) {
    printf("%s=%s\n", key, name);
}

static char *sirfsoc_get_os_type(){
	char *p, *os_type;
	char cmdline_str[BUFSIZ];
	FILE *cmdline_fp;

	cmdline_fp = fopen("/proc/cmdline", "r");
	if (cmdline_fp == NULL)  {
		return NULL;
	}
	fread(cmdline_str, 1, BUFSIZ, cmdline_fp);

	p = strstr(cmdline_str, "os_type");
	while(*p++ != '=');
	os_type = p;
	while((*p != ' ') && (*p != '\0')) p++;
	*p = '\0';
	return os_type;
}

#define RFLAG_ADDR	0x0027f000 /* 2.5MB-4096 */
#define RFLAG_START	0x5a
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

int update_bt(void){
	fprintf(stderr,"[recovery]in update_bt...\n");
	FILE*f_bt1,*f_bt2;	
	f_bt1=fopen(BT_MAC_BACKUP_PATH, "rb");
	f_bt2=fopen(BT_MAC_BACKUP_PATH2, "rb");
	
	if((f_bt1!=NULL)&&(f_bt2!=NULL)){
		fprintf(stderr,"[recovery]prepare to copy bt files ...\n");
		
		sprintf(user_part, "%sp2", target_media);//mount boot partition
		mkdir("/btmp1", 0755);
		if (mount(user_part, "/btmp1", "ext4", MS_NOATIME | MS_NODEV | MS_NODIRATIME, "") < 0) {
			fprintf(stderr, "[recovery]mount %s to /btmp1 failed!\n",user_part);
		} else{
			fprintf(stderr, "[recovery]mount %s to /btmp1 success!\n",user_part);
			copy_file(BT_MAC_BACKUP_PATH,"/btmp1/bt.inf");
			fprintf(stderr, "[recovery]copy bt.inf done\n");
			copy_file(BT_MAC_BACKUP_PATH2,"/btmp1/trail.inf");
			if(NULL!=fopen(BT_MAC_PATH3, "rb")){
				fprintf(serial_fp, "[recovery]copy trail.inf to %s done-----1111\n",user_part); 
				}
			}
	
		sprintf(user_part, "%sp3", target_media);//mount root partition
		mkdir("/btmp2", 0755);
		if (mount(user_part, "/btmp2", "ext4", MS_NOATIME | MS_NODEV | MS_NODIRATIME, "") < 0) {
			fprintf(stderr, "[recovery]mount %s to /btmp2 failed!\n",user_part);
		} else {
			fprintf(stderr, "[recovery]mount %s to /btmp2 success!\n",user_part);
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
		umount("/btmp2");
		remove("/btmp1");
		remove("/btmp2");
	}else{
		fprintf(serial_fp, "[recovery]%s or %s does not exist\n",BT_MAC_BACKUP_PATH, BT_MAC_BACKUP_PATH2);
		}
	return 0;
}

int mount_device(char *path)
{
	if (mount(path, mount_point, "vfat",MS_NOATIME | MS_NODEV | MS_NODIRATIME, "") < 0) {
			fprintf(serial_fp, "[recovery]mount %s failed!\n",path);
			return -1;
	}else{
			fprintf(serial_fp, "[recovery]mount %s as vfat successed\n",path);
		  	source_media=path;
	    }
	return 0;
}


int find_update_zip(void)
{
	if (access(UPDATE_PACKAGE_PATH, F_OK) ==0) {
		fprintf(serial_fp, "[recovery]find update.zip in udisk!\n");
		is_factory_production = true;//factory-mode means recovery from udisk or extern SD card
	}else{
		fprintf(serial_fp, "[recovery]no update.zip in udisk!\n");
		umount(mount_point);	
		sync();
		return -1;
	}
	return 0;
}

int	main(int argc, char **argv) {
	
	mkdir(mount_point, 755);//create /sdcard as the mount point,maybe we will use /mnt 
	serial_fp = fopen("/dev/ttySiRF1", "w");//A7 serial port
    if (serial_fp != NULL) fprintf(serial_fp, "[recovery]Open ttySiRF1 success!\n");
	/*redirect stdout`stderr*/
    freopen(TEMPORARY_LOG_FILE, "a", stdout); setbuf(stdout, NULL);
    freopen(TEMPORARY_LOG_FILE, "a", stderr); setbuf(stderr, NULL);

    user_partno = 7;//maybe not
    sprintf(user_part_nand, "/dev/nandblk0p%d", user_partno);
    sprintf(user_part_sd, "/dev/mmcblk0p%d", user_partno);
	
    FILE* f = NULL;
    usleep(10000);
	
    if (access(udisk, F_OK) ==0) {
		fprintf(serial_fp, "[recovery]udisk exists\n");
	/*mount udisk*/
	mount_device(udisk);
	/*find update.zip*/
	find_update_zip();
	/*find target media*/   
    if (access(nand_device, F_OK) == 0) {
        /* boot media is nand, and it will be updated*/
        target_media = nand_device;
	} else {
        /* boot media is nand, and it will be updated*/
		target_media = mmc_device;
		}
        fprintf(serial_fp, "[recovery]Recovery from %s to %s\n",source_media, target_media);
    }
	/*install packages*/

	/*******************/
	/*******************/

	else if ((f = fopen(user_part_sd, "rb")) != NULL) {
        /* sd boot and updated from its own user partition */
        target_media = "/dev/mmcblk0";
        fclose(f);
		f = NULL;
        if (mount(user_part_sd, "/sdcard", "vfat", MS_NOATIME | MS_NODEV | MS_NODIRATIME, "") < 0)
        {
		fprintf(serial_fp, "[recovery]mount %s as vfat failed\n", user_part_sd);
        }else {
        	fprintf(serial_fp, "[recovery]mount %s as vfat success\n", user_part_sd);
        }
		fprintf(serial_fp, "[recovery]Recovery from SD internal user partition to %s\n", target_media);
    } else {
        /*  nand boot and updated from its own user partition */
        target_media = "/dev/nandblk0";
        if (mount(user_part_nand, "/sdcard", "vfat", MS_NOATIME | MS_NODEV | MS_NODIRATIME, "") < 0)
        {
			fprintf(serial_fp, "[recovery]mount %s failed\n", user_part_nand);
        }else
        {	
        	fprintf(serial_fp, "[recovery]mount %s success\n", user_part_nand);	
        }
		fprintf(serial_fp, "[recovery]Recovery from Nand internal user partition to %s\n", target_media);
    }

    mkdir("/sdcard/cache", 755);
    const char *send_intent = NULL;
    const char *update_package = NULL;
    int  wipe_cache = 0;

    int status = INSTALL_SUCCESS;

/**set recovery start flag 0x5a**/
    set_flag(target_media, RFLAG_ADDR, RFLAG_START);
    fprintf(serial_fp, "\n[recovery]Set recovery start flag, read=%#x\n",get_flag(target_media, RFLAG_ADDR));

/**get update.zip path && install it**/
    update_package = UPDATE_PACKAGE_PATH;
	//status = install_package(UPDATE_PACKAGE_PATH, &wipe_cache, TEMPORARY_INSTALL_FILE);

		
/* update update.zip */
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
	}
/*restore BT files*/
	if(status == INSTALL_SUCCESS){
		fprintf(serial_fp, "[recovery]updating bt!\n");
		update_bt();
	}

	finish_recovery(send_intent);
   // sync();

/**set recovery finish flag 0xa5**/
    set_flag(target_media,RFLAG_ADDR,RFLAG_FINISH);
    fprintf(serial_fp, "\n[recovery]Set recovery finish flag, read=%#x\n",get_flag(target_media, RFLAG_ADDR));


	if (status == INSTALL_SUCCESS){
		fprintf(serial_fp, "[recovery]recovery finished\n");
	}else{ 	
		fprintf(serial_fp, "[recovery]recovery failed\n");
        fprintf(serial_fp, "[recovery]For more log information, please check %s now\n", TEMPORARY_LOG_FILE);    	 
		}
	if(0==umount("/sdcard"))
		fprintf(serial_fp, "[recovery]umount /sdcard done...\n");
    return EXIT_SUCCESS;
}
