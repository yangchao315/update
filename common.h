#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <unistd.h>
#include <signal.h>
#include <mtd/mtd-user.h>

#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <time.h>
#include <dirent.h>

#define false 0
#define true  1
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

enum { 
	INSTALL_SUCCESS, 
	INSTALL_ERROR, 
	INSTALL_CORRUPT };
	
#define LOGW(...) fprintf(stdout, "W:" __VA_ARGS__)
#define LOGI(...) fprintf(stdout, "I:" __VA_ARGS__)
	
/*log path*/
static const char *TEMPORARY_LOG_FILE = "/tmp/recovery.log";
static const char *INTENT_FILE = "/sdcard/cache/intent";
static const char *LOG_FILE = "/sdcard/cache/log";
static const char *LAST_LOG_FILE = "/sdcard/cache/last_log";

FILE* fopen_path(const char *path, const char *mode);
char *sirfsoc_get_os_type();
int pox_system(const char *cmd_line);
int compare_string(const void* a, const void* b);
void finish_recovery(const char *send_intent);
void check_and_fclose(FILE *fp, const char *name);
void copy_file(const char* source, const char* destination);
void copy_log_file(const char* source, const char* destination, int append);
