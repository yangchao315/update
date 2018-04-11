#include "common.h"

/*device path*/
char *udisk="/dev/sda1";
char *nand_device="/dev/nandblk0";
char *mmc_device="/dev/mmcblk0";
/*mount point*/
static const char *mount_point="/sdcard";
char *App_File = "/sdcard/update/APP_Update.tar.bz2";
char *Kernel_File = "/sdcard/update/Kernel_Update.tar.bz2";

#define MOUNT_KERNEL_PATH   "mount /dev/mmcblk0p2 /mnt/"
#define MOUNT_APP_PATH   "mount /dev/mmcblk0p3 /mnt/"

#define MOUNT_KERNEL_PATH2   "mount /dev/mmcblk0p6 /mnt/"
#define MOUNT_APP_PATH2   "mount /dev/mmcblk0p7 /mnt/"


/*zip package path*/
static const char *UPDATE_PACKAGE_PATH = "/sdcard/update/update.zip";
static const char *UPDATEZIP_PATH = "/user/update/update-2.zip";

/*mac_backup path*/
static const char *BT_MAC_BACKUP_PATH = "/sdcard/update/mac_backup/bt.inf";
static const char *BT_MAC_BACKUP_PATH2 = "/sdcard/update/mac_backup/trail.inf";
/*mac restore path*/
static const char *BT_MAC_PATH="/boot/bt.inf";
static const char *BT_MAC_PATH2="/root/var/lib/trail/bt.inf";
static const char *BT_MAC_PATH3="/boot/trail.inf";
static const char *BT_SYNERGY_PATH="/btmp2/var/lib/csr_synergy/bt.inf";


#define COMMIT_FLAG_ADDR 0x200000
char *Uboot_Name = "/sdcard/update/u-boot.csr";
char *Uboot_Path = "/dev/mmcblk0";

char *M3_File = "/sdcard/update/myrtos.bin";
char *M3_Path= "/dev/mtd0";

FILE *serial_fp = NULL;
int user_partno = 0;
bool is_factory_production = false;
const char *target_media = NULL;
const char *source_media = NULL;
char user_part[16],user_part_nand[16], user_part_sd[16], ext_user_part_sd[16];





