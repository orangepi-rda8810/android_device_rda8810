#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <termios.h>
#include <string.h>

struct bin_tag {
	unsigned int    iOffset;
	int    iFileSize;
	int    iPackSize;
	unsigned int    startAddr;
	char   szTitle[128];
	char   szPartName[128];
	char   szFilePath[1024];
	int    bAddress;
	int    bNeedExe;
};
static struct bin_tag tag;

int main(int argc, char **argv)
{
	int str;
	char *file_path = NULL;
	int file_size = 0;
	int packet_size = 128 * 1024;
	char image_name[50] = {'\0'};
	int executable = 0;
	unsigned int start_addr = 0;
	unsigned int offset = 0;
	int image_cnt = 0;

	while((str=getopt(argc,argv,"f:a:p:i:o:s:c:x:"))!= EOF) {
		switch (str) {
		case 'f':
			file_path = optarg;
			//printf("write to  %s\n", file_path);
			break;
		case 'a':
			start_addr = strtoul(optarg, NULL, 16);
			//printf("image's addr %x\n", start_addr);
			break;
		case 'p':
			packet_size = strtoul(optarg, NULL, 16);
			//printf("packet size  set to %d\n", packet_size);
			break;
		case 'i':
			strcpy(image_name, optarg);
			//printf("image name %s\n", image_name);
			break;
		case 'o':
			offset = strtoul(optarg, NULL, 10);
			//printf("offset is %d\n", offset);
			break;
		case 's':
			file_size = strtoul(optarg, NULL, 10);
			//printf("file size is %d\n", file_size);
			break;
		case 'c':
			image_cnt = strtoul(optarg, NULL, 10);
			//printf("images count is %d\n", image_cnt);
			break;
		case 'x':
			executable = strtoul(optarg, NULL, 10);
			//printf("file is executable %d\n", executable);
			break;
		case '?':
			printf("\n usage: build_header -f file path \n");
			printf(" -f, output file path \n");
			printf(" -a, image address \n");
			printf(" -p, packet size\n");
			printf(" -i, download name\n");
			printf(" -x, image is excutable\n");
			printf(" -c, images count\n");
			printf(" -o, images offset\n");
			exit(1);
		default:
			break;
		}
        }

	int fd = open(file_path, O_RDWR);
	if (fd < 0) {
		printf("can not load file %s\n", file_path);
		exit(-1);
	}

	//check the arguments

	tag.iOffset = offset;
	tag.iFileSize = file_size;
	tag.iPackSize = (packet_size / 1024);
	tag.startAddr = start_addr;
	strncpy(&tag.szTitle[0], image_name, 128-1);
	strncpy(&tag.szPartName[0], image_name, 128-1);
	tag.szFilePath[0] = '\0';
	tag.bAddress = executable;
	tag.bNeedExe = executable;

	off_t pos = lseek(fd, 0, SEEK_SET);
	if (pos == -1) {
		printf("seek file fails %s\n", strerror(errno));
		exit(-1);
	}
	write(fd, &image_cnt, sizeof(int));

	pos = lseek(fd, 0, SEEK_END);
	if (pos == -1) {
		printf("seek file fails %s\n", strerror(errno));
		exit(-1);
	}
	write(fd, &tag, sizeof(tag));

	close(fd);
	return 0;
}
