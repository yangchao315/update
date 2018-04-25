#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <linux/fs.h>
#include <mtd/mtd-user.h>
#include <stdbool.h>

#define VAL_STRING  1  // data will be NULL-terminated; size doesn't count null
#define VAL_BLOB    2
#define COMMIT_FLAG_ADDR 0x200000
//char *Uboot_Name = "/media/disk/update/u-boot.csr";
//char *Uboot_Path = "/dev/nandblk0";
typedef struct {
    int type;
    ssize_t size;
    char* data;
} Value;

Value* StringValue(char* str) {
    if (str == NULL) return NULL;
    Value* v = malloc(sizeof(Value));
    v->type = VAL_STRING;
    v->size = strlen(str);
    v->data = str;
    return v;
}
/*write_uboot("/tmp/u-boot.csr","/dev/nandblk0")!="/dev/nandblk0",*/
// write_uboot(file, partition)
Value* WriteUbootFn(char* partition,char* filename) {
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

    printf("%s %s partition from %s\n",
           success ? "wrote" : "failed to write", partition, filename);

    result = success ? partition : strdup("");

done:
    if (result != partition) {
		free(partition);
    	free(filename);
		}
	fprintf(stderr, "[WriteUbootFn]write u-boot.csr successed! \n");
    return StringValue(result);
}

void print_usage(void)
{
	printf("Usage:\n");
	printf("NAND version:\n./WriteUbootFn /dev/nandblk0 u-boot.csr\n");
	printf("eMMC version:\n./WriteUbootFn /dev/mmcblk0 u-boot.csr\n");
}
int main(int argc, char *argv[])
{
    if(argc!=3){
        printf("wrong num of args!\n");
	    print_usage();
        return -1;
    }else{
	    WriteUbootFn(argv[1],argv[2]);
    }
return 0;
}
