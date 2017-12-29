#include "common.h"

/*device path*/
char *udisk="/dev/sda1";
char *nand_device="/dev/nandblk0";
char *mmc_device="/dev/mmcblk0";
/*mount point*/
static const char *mount_point="/sdcard";
char *App_File = "/sdcard/update/APP_Update.tar.bz2";
char *Kernel_File = "/sdcard/update/Kernel_Update.tar.bz2";

#ifdef EMMC_VER
#define	MOUNT_APP_PATH   "mount /dev/mmcblk0p3 /mnt/"
#define MOUNT_KERNEL_PATH   "mount /dev/mmcblk0p2 /mnt/"
#else
#define MOUNT_APP_PATH   "mount /dev/nandblk0p3 /mnt/"
#define MOUNT_KERNEL_PATH   "mount /dev/nandblk0p2 /mnt/"
#endif


/*zip package path*/
static const char *UPDATE_PACKAGE_PATH = "/sdcard/update/update.zip";
static const char *UPDATEZIP_PATH = "/user/update/update.zip";

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
char *Uboot_Path = "/dev/nandblk0";

FILE *serial_fp = NULL;
int user_partno = 0;
bool is_factory_production = false;
const char *target_media = NULL;
const char *source_media = NULL;
char user_part[16],user_part_nand[16], user_part_sd[16];





