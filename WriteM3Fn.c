#include "common.c"
#include <stdint.h>
#include <sys/ioctl.h>
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
#include <unistd.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/mount.h>

char *M3_File = "/media/disk/update/myrtos.bin";
char *M3_Path= "/dev/mtd0";

static int erase_flash(int fd, int offset, int bytes)
{
	int err;
	struct erase_info_user erase;
	erase.start = offset;
	erase.length = bytes;
	err = ioctl(fd,MEMERASE, &erase);
	if (err < 0) {
		perror("MEMERASE");
		return 1;
	}
	fprintf(stderr, "Erased %d bytes from address 0x%.8x in flash\n", bytes, offset);
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
		perror("lseek()");
		return 1;
	}
	if ((fp = fopen(filename, "r")) == NULL) {
		perror("fopen()");
		return 1;
	}
retry:
	if ((buf = (u_int8_t *) malloc(size)) == NULL) {
		fprintf(stderr, "%s: malloc(%#x) failed\n", __func__, size);
		if (size != BUFSIZ) {
			size = BUFSIZ;
			fprintf(stderr, "%s: trying buffer size %#x\n", __func__, size);
			goto retry;
		}
		perror("malloc()");
		fclose(fp);
		return 1;
	}
	do {
		if (n <= size)
			size = n;
		if (fread(buf, size, 1, fp) != 1 || ferror(fp)) {
			fprintf(stderr, "%s: fread, size %#x, n %#x\n", __func__, size, n);
			perror("fread()");
			free(buf);
			fclose(fp);
			return 1;
		}
		err = write(fd, buf, size);
		if (err < 0) {
			fprintf(stderr, "%s: write, size %#x, n %#x\n", __func__, size, n);
			perror("write()");
			free(buf);
			fclose(fp);
			return 1;
		}
		n -= size;
	} while (n > 0);

	if (buf != NULL)
		free(buf);
	fclose(fp);
	printf("Copied %d bytes from %s to address 0x%.8x in flash\n", len, filename, offset);
	return 0;
}


char* WriteM3Fn(char* filename,char* partition) {
	char* result = NULL;
	int rfd, wfd;
	int rf_size = 0, err = 0;
	struct stat st;
	bool success = true;

    if (strlen(partition) == 0) {
        fprintf(stderr, "[WriteM3Fn] partition argument to %s can't be empty", partition);
        goto done;
    }
    if (strlen(filename) == 0) {
        fprintf(stderr, "[WriteM3Fn] file argument to %s can't be empty", filename);
        goto done;
    }

	if ((rfd = open(filename, O_SYNC | O_RDONLY)) < 0) {
		fprintf(stderr, "open %s failed\n",filename);
        	result = strdup("");
		goto done;
	}

	if ((wfd = open(partition, O_SYNC | O_RDWR)) < 0) {
		fprintf(stderr, "open %s failed\n",partition);
        	result = strdup("");
		goto done;
	}

	stat(filename, &st);
	rf_size = st.st_size;

	fprintf(stderr, "erase flash...\n");
	err = erase_flash(wfd, 0, rf_size);
	if (err) {
		fprintf(stderr, "erase SPI NOR failed\n");
		success = false;
		goto done;
	}
	fprintf(stderr, "%s (%d bytes) be worte\n", filename, rf_size);
	err = file_to_flash(wfd, 0, rf_size, filename);
	if (err) {
		fprintf(stderr, "wirte Freertos to SPI NOR failed\n");
		success = false;
		goto done;
	}

	/* close device */
	if (close(rfd) < 0)
		fprintf(stderr, "close rfd failed\n");
	if (close(wfd) < 0)
		fprintf(stderr, "close wfd failed\n");

	result = success ? partition : strdup("");

done:
    if (result != partition) {
		free(partition);
		free(filename);
		}
	fprintf(stderr, "[WriteM3Fn]write M3 successed! \n");
    return result;
}


int main(int argc,char **argv)
{	
	WriteM3Fn(M3_File,M3_Path);
	return 0;
}

