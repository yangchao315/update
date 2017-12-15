
/*log path*/
static const char *TEMPORARY_LOG_FILE = "/tmp/recovery.log";
static const char *TEMPORARY_INSTALL_FILE = "/tmp/last_install";
static const char *INTENT_FILE = "/sdcard/cache/intent";
static const char *LOG_FILE = "/sdcard/cache/log";
static const char *LAST_LOG_FILE = "/sdcard/cache/last_log";
static const char *LAST_INSTALL_FILE = "/sdcard/cache/last_install";

/*zip package path*/
static const char *UPDATE_PACKAGE_PATH = "/sdcard/update/update.zip";
static const char *UPDATE_PACKAGE_PATH_EXT = "/sdcard/card/update.zip";
static const char *UPDATE_PACKAGE_PATH_UDISK = "/udisk/update/update.zip";
static const char *UPDATEZIP_PATH = "/user/update/update.zip";

/*mac_backup path*/
static const char *BT_MAC_BACKUP_PATH = "/sdcard/update/mac_backup/bt.inf";
static const char *BT_MAC_BACKUP_PATH2 = "/sdcard/update/mac_backup/trail.inf";
/*mac restore path*/
static const char *BT_MAC_PATH="/boot/bt.inf";
static const char *BT_MAC_PATH2="/root/var/lib/trail/bt.inf";
static const char *BT_MAC_PATH3="/boot/trail.inf";
static const char *BT_SYNERGY_PATH="/btmp2/var/lib/csr_synergy/bt.inf";

FILE *serial_fp = NULL;
int user_partno = 0;
bool is_factory_production = false;
const char *target_media = NULL;
const char *source_media = NULL;
char user_part[16],user_part_nand[16], user_part_sd[16], ext_user_part_sd[16];
enum { INSTALL_SUCCESS, INSTALL_ERROR, INSTALL_CORRUPT };

FILE* fopen_path(const char *path, const char *mode);


