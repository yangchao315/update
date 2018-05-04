#include "common.h"
#define COMMIT_FLAG_ADDR 0x200000
#define MOUNT_KERNEL_PATH   "mount /dev/mmcblk0p2 /mnt/"
#define MOUNT_APP_PATH   "mount /dev/mmcblk0p3 /mnt/"
#define FORMAT_ROOT   "mke2fs -T ext4 -L root /dev/mmcblk0p3"

#define MOUNT_KERNEL_PATH2   "mount /dev/mmcblk0p6 /mnt/"
#define MOUNT_APP_PATH2   "mount /dev/mmcblk0p7 /mnt/"
#define FORMAT_ROOT2   "mke2fs -T ext4 -L root2 /dev/mmcblk0p7"

/*device path*/
char *udisk="/dev/sda1";
char *nand_device="/dev/nandblk0";
char *mmc_device="/dev/mmcblk0";
/*mount point*/
static const char *mount_point="/sdcard";


char *Uboot_File = "/sdcard/update/u-boot.csr";
char *Uboot_Path = "/dev/mmcblk0";
char *M3_File = "/sdcard/update/myrtos.bin";
char *M3_Path= "/dev/mtd0";

char *Kernel_File = "/sdcard/update/Kernel_Update.tar.bz2";
char *App_File = "/sdcard/update/APP_Update.tar.bz2";
char *Rootfs_File = "/sdcard/update/rootfs.tar.gz";

/*zip package path*/
static const char *UPDATE_PACKAGE_PATH = "/sdcard/update/update.zip";
static const char *UPDATEZIP_PATH = "/user/update/update-2.zip";

/*BT mac address backup path*/
static const char *BT_MAC_BACKUP_PATH = "/sdcard/update/mac_backup/bt.inf";
static const char *BT_MAC_BACKUP_PATH2 = "/sdcard/update/mac_backup/trail.inf";
/*BT mac address restore path*/
static const char *BT_MAC_PATH="/boot/bt.inf";
static const char *BT_MAC_PATH2="/root/var/lib/trail/bt.inf";
static const char *BT_MAC_PATH3="/boot/trail.inf";
static const char *BT_SYNERGY_PATH="/btmp2/var/lib/csr_synergy/bt.inf";

FILE *serial_fp = NULL;
int user_partno = 0;
bool is_factory_production = false;
const char *target_media = NULL;
const char *source_media = NULL;

char user_part[16],user_part_nand[16];

enum UpdateCheckBit {
    UCB_UBOOT,
    UCB_KERNEL,
    UCB_APP,
    UCB_ROOTFS,
    UCB_M3,
    UCB_RECOVERY,
    UCB_MAX
};
