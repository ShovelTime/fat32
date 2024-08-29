#define FUSE_USE_VERSION 30
#include <fuse3/fuse.h>
#include <fuse3/fuse_lowlevel.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

const unsigned short SECTOR_SIZE = 512; // true across every fat32 filesystem, if this changes something is VERY wrong;
const unsigned char FAT_NUMBER = 2; // What does that even mean?
const unsigned short MAGIC_SIG = 0xAA55; // Magic number identifying the volume ID sector.

struct fat32_descriptor
{
	unsigned char sec_per_cluster; // 0x0D
	unsigned short reserved_sectors_amnt; // 0x0E, default is typically 0x20;
	unsigned int sec_per_fat; //0x24
	unsigned int root_first_cluster; //0x2C //usually at 0x2;
};

int parse_vsector(const struct fat32_descriptor* container, const char* sector)
{
	unsigned short magic_num = *((short*)(sector + 0x1FE));
	if(magic_num != MAGIC_SIG)
	{
		printf("Invalid volume sector!\n");
		return 1; // ma
	}
	
	return 0;
}

int main(int argc, char* argv[])
{
	if(argc < 2)
	{
		printf("E:Device file argument expected. \n");
		return 1;
	}
	char* path = argv[1];
	printf("%s \n", path); 
	unsigned char buffer[512];
	int fd = open(path, O_RDONLY, O_SYNC);
	if (errno != 0)
	{
		int fail = errno;
		printf("Failed to open device! error code is %d \n", fail);
		return fail;
	}
	read(fd, buffer, 512);

	close(fd);
	//close(fd);
	return 0;
	

}
