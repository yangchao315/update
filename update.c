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
//#include "bootloader.h"
//#include "common.h"
//#include "cutils/properties.h"
//#include "cutils/android_reboot.h"
//#include "install.h"
//#include "minui/minui.h"
//#include "minzip/DirUtil.h"
//#include "roots.h"
//#include "recovery_ui.h"
#define LOGW(...) fprintf(stdout, "W:" __VA_ARGS__)
#define printf(...) fprintf(stdout, "I:" __VA_ARGS__)
#define false 0
#define true  1


static const struct option OPTIONS[] = {
  { "send_intent", required_argument, NULL, 's' },
  { "update_package", required_argument, NULL, 'u' },
  { "wipe_data", no_argument, NULL, 'w' },
  { "wipe_cache", no_argument, NULL, 'c' },
  { "show_text", no_argument, NULL, 't' },
  { NULL, 0, NULL, 0 },
};
typedef struct {
    const char* mount_point;  // eg. "/cache".  must live in the root directory.

    const char* fs_type;      // "yaffs2" or "ext4" or "vfat"

    const char* device;       // MTD partition name if fs_type == "yaffs"
                              // block device if fs_type == "ext4" or "vfat"

    const char* device2;      // alternative device to try if fs_type
                              // == "ext4" or "vfat" and mounting
                              // 'device' fails

    long long length;         // (ext4 partition only) when
                              // formatting, size to use for the
                              // partition.  0 or negative number
                              // means to format all but the last
                              // (that much).
} Volume;

static const char *COMMAND_FILE = "/sdcard/cache/recovery/command";
static const char *INTENT_FILE = "/sdcard/cache/recovery/intent";
static const char *LOG_FILE = "/sdcard/cache/recovery/log";
static const char *LAST_LOG_FILE = "/sdcard/cache/recovery/last_log";
static const char *LAST_INSTALL_FILE = "/sdcard/cache/recovery/last_install";
static const char *CACHE_ROOT = "/sdcard/cache";
static const char *SDCARD_ROOT = "/sdcard";
static const char *TEMPORARY_LOG_FILE = "/tmp/recovery.log";
static const char *TEMPORARY_INSTALL_FILE = "/tmp/last_install";
static const char *SIDELOAD_TEMP_DIR = "/tmp/sideload";

static const char *UPDATE_PACKAGE_PATH = "/sdcard/update/update.zip";
static const char *UPDATE_PACKAGE_PATH_EXT = "/sdcard/card/update.zip";
static const char *UPDATE_PACKAGE_PATH_UDISK = "/udisk/update/update.zip";
static const char *UPDATEZIP_PATH = "/user/update/update.zip";

//added by yangchao 20160923
static const char *BT_MAC_BACKUP_PATH = "/sdcard/update/mac_backup/bt.inf";
static const char *BT_MAC_BACKUP_PATH2 = "/sdcard/update/mac_backup/trail.inf";

static const char *BT_MAC_PATH="/boot/bt.inf";
static const char *BT_MAC_PATH2="/root/var/lib/trail/bt.inf";
static const char *BT_MAC_PATH3="/boot/trail.inf";
static const char *BT_SYNERGY_PATH="/btmp2/var/lib/csr_synergy/bt.inf";
//end


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

/*
 * The recovery tool communicates with the main system through /cache files.
 *   /cache/recovery/command - INPUT - command line for tool, one arg per line
 *   /cache/recovery/log - OUTPUT - combined log file from recovery run(s)
 *   /cache/recovery/intent - OUTPUT - intent that was passed in
 *
 * The arguments which may be supplied in the recovery.command file:
 *   --send_intent=anystring - write the text out to recovery.intent
 *   --update_package=path - verify install an OTA package file
 *   --wipe_data - erase user data (and cache), then reboot
 *   --wipe_cache - wipe cache (but not user data), then reboot
 *   --set_encrypted_filesystem=on|off - enables / diasables encrypted fs
 *
 * After completing, we remove /cache/recovery/command and reboot.
 * Arguments may also be supplied in the bootloader control block (BCB).
 * These important scenarios must be safely restartable at any point:
 *
 * FACTORY RESET
 * 1. user selects "factory reset"
 * 2. main system writes "--wipe_data" to /cache/recovery/command
 * 3. main system reboots into recovery
 * 4. get_args() writes BCB with "boot-recovery" and "--wipe_data"
 *    -- after this, rebooting will restart the erase --
 * 5. erase_volume() reformats /data
 * 6. erase_volume() reformats /cache
 * 7. finish_recovery() erases BCB
 *    -- after this, rebooting will restart the main system --
 * 8. main() calls reboot() to boot main system
 *
 * OTA INSTALL
 * 1. main system downloads OTA package to /cache/some-filename.zip
 * 2. main system writes "--update_package=/cache/some-filename.zip"
 * 3. main system reboots into recovery
 * 4. get_args() writes BCB with "boot-recovery" and "--update_package=..."
 *    -- after this, rebooting will attempt to reinstall the update --
 * 5. install_package() attempts to install the update
 *    NOTE: the package install must itself be restartable from any point
 * 6. finish_recovery() erases BCB
 *    -- after this, rebooting will (try to) restart the main system --
 * 7. ** if install failed **
 *    7a. prompt_and_wait() shows an error icon and waits for the user
 *    7b; the user reboots (pulling the battery, etc) into the main system
 * 8. main() calls maybe_install_firmware_update()
 *    ** if the update contained radio/hboot firmware **:
 *    8a. m_i_f_u() writes BCB with "boot-recovery" and "--wipe_cache"
 *        -- after this, rebooting will reformat cache & restart main system --
 *    8b. m_i_f_u() writes firmware image into raw cache partition
 *    8c. m_i_f_u() writes BCB with "update-radio/hboot" and "--wipe_cache"
 *        -- after this, rebooting will attempt to reinstall firmware --
 *    8d. bootloader tries to flash firmware
 *    8e. bootloader writes BCB with "boot-recovery" (keeping "--wipe_cache")
 *        -- after this, rebooting will reformat cache & restart main system --
 *    8f. erase_volume() reformats /cache
 *    8g. finish_recovery() erases BCB
 *        -- after this, rebooting will (try to) restart the main system --
 * 9. main() calls reboot() to boot main system
 */

static const int MAX_ARG_LENGTH = 4096;
static const int MAX_ARGS = 100;

// open a given path, mounting partitions as necessary
FILE*
fopen_path(const char *path, const char *mode) {
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

    // Reset to mormal system boot so recovery won't cycle indefinitely.
/**
	struct bootloader_message boot;
    memset(&boot, 0, sizeof(boot));
    set_bootloader_message(&boot);
**/
    // Remove the command file, so recovery won't repeat indefinitely.
    /*
    if (ensure_path_mounted(COMMAND_FILE) != 0 ||
        (unlink(COMMAND_FILE) && errno != ENOENT)) {
        LOGW("Can't unlink %s\n", COMMAND_FILE);
    }

    ensure_path_unmounted(CACHE_ROOT);
    sync();  // For good measure.*/
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


int
main(int argc, char **argv) {
    time_t start = time(NULL);
    FILE *serial_fp = NULL;
    serial_fp = fopen("/dev/ttySiRF1", "w");
	mkdir("/sdcard", 755);
    int ret = 0;
    int user_partno = 0;
    bool is_factory_production = false;

    if (serial_fp != NULL) fprintf(serial_fp, "[recovery]Starting recovery\n");
    // If these fail, there's not really anywhere to complain...
    freopen(TEMPORARY_LOG_FILE, "a", stdout); setbuf(stdout, NULL);
    freopen(TEMPORARY_LOG_FILE, "a", stderr); setbuf(stderr, NULL);
    printf("Starting recovery");

    // mount user partition /
    //bool is_factory_production = false;
    const char *target_media = NULL;
	const char *source_media = NULL;
    char user_part_nand[16], user_part_sd[16], ext_user_part_sd[16];
    char user_part[16];
    /*
    char *os_type = sirfsoc_get_os_type();
    if (!strncmp(os_type, "linux", 5)) user_partno = 6;
    if (!strncmp(os_type, "android", 8)) user_partno = 8;
    */
    user_partno = 7;
    sprintf(user_part_nand, "/dev/nandblk0p%d", user_partno);
    sprintf(user_part_sd, "/dev/mmcblk0p%d", user_partno);
    sprintf(ext_user_part_sd, "/dev/mmcblk1p1");
    FILE* f = NULL;
    usleep(10000);
    if ((f = fopen("/dev/sda", "rb")) != NULL) {
	fprintf(serial_fp, "[recovery]UDISK exists\n");
        fclose(f);
	f = NULL;
		
	if (mount("/dev/sda1", "/sdcard", "vfat", MS_NOATIME | MS_NODEV | MS_NODIRATIME, "") < 0) {
	    fprintf(serial_fp, "[recovery]mount /dev/sda1 as vfat failed\n");
	    if (mount("/dev/sda", "/sdcard", "vfat", MS_NOATIME | MS_NODEV | MS_NODIRATIME, "") < 0) {
	    fprintf(serial_fp, "[recovery]mount /dev/sda as vfat failed\n");
	     } else {
	             fprintf(serial_fp, "[recovery]mount /dev/sda as vfat successed\n");
				 source_media="UDISK(vfat)";
	     		}
	}else{
	      fprintf(serial_fp, "[recovery]mount /dev/sda1 as vfat successed\n");
		source_media="UDISK(vfat)";
	    }
		
	    if ((f = fopen(UPDATE_PACKAGE_PATH, "rb")) == NULL) {
	        fprintf(serial_fp, "[recovery]%s does not exist\n",UPDATE_PACKAGE_PATH);
	        umount("/sdcard");
	        is_factory_production = false;
	        if ((f = fopen(user_part_nand, "rb")) != NULL) {
	            if (mount(user_part_nand, "/sdcard", "vfat", MS_NOATIME | MS_NODEV | MS_NODIRATIME, "") < 0)
	            {
	               fprintf(serial_fp,"[recovery]mount %s as vfat failed\n", user_part_nand);
	            }else {
	            	fprintf(serial_fp,"[recovery]mount %s as vfat success\n", user_part_nand);
			source_media=user_part_nand;
	            }
	            fclose(f);
	            target_media = "/dev/nandblk0";
	        } else if ((f = fopen(user_part_sd, "rb")) != NULL) {
	            if (mount(user_part_sd, "/sdcard", "vfat", MS_NOATIME | MS_NODEV | MS_NODIRATIME, "") < 0)
	            {
			fprintf(serial_fp,"[recovery]mount %s as vfat failed\n", user_part_sd);
	            }else {
	            	fprintf(serial_fp,"[recovery]mount %s as vfat success\n", user_part_sd);
			source_media=user_part_sd;
	            }
	            fclose(f);
	            target_media = "/dev/mmcblk0";
	        }
	    }

        if ((f = fopen("/dev/nandblk0", "rb")) != NULL) {
            /* nand is inserted */
            /* boot media is udisk, and nand in slot0 will be updated*/
            target_media = "/dev/nandblk0";
	    	fclose(f);
        } else {
            target_media = "/dev/mmcblk0";
		}
        fprintf(serial_fp, "[recovery]Recovery from %s to %s\n",source_media, target_media);
    } else if ((f = fopen(ext_user_part_sd, "rb")) != NULL) {
        /* updated from sd2 to sd0*/
        is_factory_production = true;
        target_media = "/dev/mmcblk0";
		fclose(f);
		f = NULL;
        if (mount(ext_user_part_sd, "/sdcard", "vfat", MS_NOATIME | MS_NODEV | MS_NODIRATIME, "") < 0)
        {
			fprintf(serial_fp,"[recovery]mount %s as vfat failed\n", ext_user_part_sd);
        }else {
			fprintf(serial_fp,"[recovery]mount %s as vfat success\n", ext_user_part_sd);
		}
        fprintf(serial_fp, "[recovery]Recovery from external SD to %s\n", target_media);
    } else if ((f = fopen(user_part_sd, "rb")) != NULL) {
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
        /* boot media is nand */
        target_media = "/dev/nandblk0";
        if (mount(user_part_nand, "/sdcard", "vfat", MS_NOATIME | MS_NODEV | MS_NODIRATIME, "") < 0)
        {
			fprintf(serial_fp, "[recovery]mount %s failed\n", user_part_nand);
        }else
        {	
        	fprintf(serial_fp, "[recovery]mount %s success\n", user_part_nand);	
        }
        if ((f = fopen("/dev/mmcblk0p1", "rb")) != NULL) {
            /* sd card is inserted to update nand */
            is_factory_production = true;
            fclose(f);
	    	f = NULL;
            umount("/sdcard");
			//modify by yangchangwen for sdcard mount or fail to mount
            if (mount("/dev/mmcblk0p1", "/sdcard", "vfat", MS_RDONLY|MS_NOATIME | MS_NODEV | MS_NODIRATIME, "") < 0)
            {
				fprintf(serial_fp, "[recovery]mount /dev/mmcblk0p1 failed\n");
            }else {
				fprintf(serial_fp, "[recovery]mount /dev/mmcblk0p1 success\n");
			}
            fprintf(serial_fp, "[recovery]Recovery from external SD to NAND, %s", ctime(&start));
        }/* else there is no sd card inserted */
		fprintf(serial_fp, "[recovery]Recovery from Nand internal user partition to %s, %s", target_media,ctime(&start));
    }

    mkdir("/sdcard/cache", 755);
    int previous_runs = 0;
    const char *send_intent = NULL;
    const char *update_package = NULL;
    int wipe_data = 0, wipe_cache = 0;
    const char *wifi_bt_address_path = NULL;

    int status = INSTALL_SUCCESS;

    set_flag(target_media, RFLAG_ADDR, RFLAG_START);
    fprintf(serial_fp, "[recovery]Set recovery start flag, read=%#x\n",
		    get_flag(target_media, RFLAG_ADDR));
        //status = INSTALL_ERROR;  // No command specified
        // No command specified
        // do default operation: install update.zip from /sdcard/card or /sdcard
	fprintf(serial_fp, "[recovery]using /sdcard/update/update.zip\n");
        update_package = UPDATE_PACKAGE_PATH;
        //status = install_package(UPDATE_PACKAGE_PATH, &wipe_cache, TEMPORARY_INSTALL_FILE);
	if(status != INSTALL_SUCCESS) {  
		fprintf(serial_fp, "[recovery]no valid update.zip\n");
		fprintf(serial_fp, "[recovery]prepare update.zip in external SD or internal user partition!\n");
		fprintf(serial_fp, "[recovery]Then retry!\n");
	}
	/* update update.zip */
	if(status == INSTALL_SUCCESS) {
            sprintf(user_part, "%sp7", target_media);
	    mkdir("/user", 0755);
            if (mount(user_part, "/user", "vfat", MS_NOATIME | MS_NODEV | MS_NODIRATIME, "") < 0) {
		fprintf(serial_fp, "[recovery]mount %s to /user failed!\n",user_part);
	    } else {
		mkdir("/user/update",0755);
		copy_file(update_package, UPDATEZIP_PATH);
	        fprintf(serial_fp, "[recovery] copy update.zip done!\n");
	    }
	}

	if(status == INSTALL_SUCCESS){
	FILE* f_bt1;
	FILE* f_bt2;

	f_bt1=fopen(BT_MAC_BACKUP_PATH, "rb");
	f_bt2=fopen(BT_MAC_BACKUP_PATH2, "rb");
	if((f_bt1!=NULL)&&(f_bt2!=NULL)){
		fprintf(serial_fp,"[recovery]prepare to copy bt files ...\n");
		sprintf(user_part, "%sp2", target_media);//mount boot partition
	    mkdir("/btmp1", 0755);
            if (mount(user_part, "/btmp1", "ext4", MS_NOATIME | MS_NODEV | MS_NODIRATIME, "") < 0) {
		fprintf(serial_fp, "[recovery]mount %s to /btmp1 failed!\n",user_part);
	    } else {
			fprintf(serial_fp, "[recovery]mount %s to /btmp1 success!\n",user_part);
			copy_file(BT_MAC_BACKUP_PATH,"/btmp1/bt.inf");
			fprintf(serial_fp, "[recovery]copy bt.inf done\n");
			copy_file(BT_MAC_BACKUP_PATH2,"/btmp1/trail.inf");
			if(NULL!=fopen(BT_MAC_PATH3, "rb"))
			{
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
			if(NULL!=fopen(BT_MAC_PATH2, "rb"))
			{
			fprintf(serial_fp, "[recovery]copy trail.inf to %s done-----1111\n",user_part);	
			}
			mkdir("/btmp2/var/lib/csr_synergy",0755);
			copy_file(BT_MAC_BACKUP_PATH,"/btmp2/var/lib/csr_synergy/bt.inf");
			if(NULL!=fopen(BT_SYNERGY_PATH, "rb"))
			{
			fprintf(serial_fp, "[recovery]copy bt.inf to %s done-----2222\n",user_part);
			}
	    }
		fclose(f_bt1);
		fclose(f_bt2);
	}else{
		fprintf(serial_fp, "[recovery]%s or %s does not exist\n",BT_MAC_BACKUP_PATH, BT_MAC_BACKUP_PATH2);
		}
	}

        if(status != INSTALL_SUCCESS) {
	fprintf(serial_fp, "[recovery]Recovery failed\n");
        } else {
		fprintf(serial_fp, "[recovery]Recovery finished\n");
        }
    // Otherwise, get ready to boot the main system...
   // finish_recovery(send_intent);
   // sync();
    set_flag(target_media,RFLAG_ADDR,RFLAG_FINISH);
    fprintf(serial_fp, "[recovery]Set recovery finish flag, read=%#x\n",get_flag(target_media, RFLAG_ADDR));

	if (status == INSTALL_SUCCESS){
		fprintf(serial_fp, "[recovery]recovery finished\n");
		}else{ 	fprintf(serial_fp, "[recovery]recovery failed\n");
            	fprintf(serial_fp, "[recovery]For more log information, please check %s now\n", TEMPORARY_LOG_FILE);    	 
	    }
	umount("/sdcard");
	fprintf(serial_fp, "[recovery]umount UDISK done...\n");
    return EXIT_SUCCESS;
}
