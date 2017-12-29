#include "common.h"

int compare_string(const void* a, const void* b) {
    return strcmp(*(const char**)a, *(const char**)b);
}

int pox_system(const char *cmd_line)
{
    int ret = 0;
    __sighandler_t old_handler;

    old_handler = signal(SIGCHLD, SIG_DFL);
    ret = system(cmd_line);
    signal(SIGCHLD, old_handler);
    return ret;
}


void check_and_fclose(FILE *fp, const char *name) {
    fflush(fp);
    if (ferror(fp)) printf("Error in %s\n(%s)\n", name, strerror(errno));
    fclose(fp);
}

void copy_file(const char* source, const char* destination) {
    char buf[4096];
    int bytes_in, bytes_out;
    int src_len = 0;
    int dst_len = 0;
    FILE *dst = fopen(destination, "wb");
    if (dst == NULL) {
        printf("[copy_file]Can't open %s\n", destination);
    } else {
        FILE *src = fopen(source, "rb");
        if (src != NULL) {
            fseek(src, 0, SEEK_SET);  // Since last write
	    while ((bytes_in = fread(buf, 1, 4096, src)) > 0 ) {
                src_len += bytes_in;
                bytes_out = fwrite (buf, 1, bytes_in, dst);
		dst_len += bytes_out;
            }
	    printf("[copy_file]Copy %d, Copied %d\n", src_len, dst_len);
            check_and_fclose(src, source);
        }
        check_and_fclose(dst, destination);
    }
}

char *sirfsoc_get_os_type(){
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
// open a given path, mounting partitions as necessary
FILE* fopen_path(const char *path, const char *mode) {
    FILE *fp = fopen(path, mode);
    return fp;
}

// How much of the temp log we have copied to the copy in cache.
long tmplog_offset = 0;

void copy_log_file(const char* source, const char* destination, int append) {
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

void finish_recovery(const char *send_intent) {
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
    chmod(LOG_FILE, 0600);
    chmod(LAST_LOG_FILE, 0640);
}




