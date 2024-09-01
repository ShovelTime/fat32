#define FUSE_USE_VERSION 30
#include <fuse3/fuse.h>
#include <fuse3/fuse_lowlevel.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

const unsigned short SECTOR_SIZE = 512; // true across every fat32 filesystem, if this changes something is VERY wrong;
const unsigned char FAT_NUMBER = 2; // What does that even mean?
const unsigned short MAGIC_SIG = 0xAA55; // Magic number identifying the volume ID sector. Offset at 0x1FE in vsector.

struct fat32_descriptor
{
	unsigned char sec_per_cluster; // 0x0D
	unsigned short reserved_sectors_amnt; // 0x0E, default is typically 0x20;
	unsigned int sec_per_fat; //0x24
	unsigned int root_first_cluster; //0x2C //usually at 0x2;
};


int parse_vsector(struct fat32_descriptor* const container, const unsigned char* const sector)
{
	unsigned short magic_num = *((short*)(sector + 0x1FE));
	if(magic_num != MAGIC_SIG)
	{
		printf("Invalid volume sector!\n");
		return 1; // ma
	}
	
	container->sec_per_cluster = sector[0x0D];
	container->reserved_sectors_amnt = *((unsigned short*)(sector + 0x0E));
	container->sec_per_fat = *((unsigned int*)(sector + 0x24));
	container->root_first_cluster = *((unsigned int*)(sector + 0x2C));

	return 0;
}

int main(int argc, char* argv[])
{
	if(argc < 2)
	{
		printf("E:Device file argument expected. \n");
		return 1;
	}
	const char* path = argv[1];
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
	

	struct fat32_descriptor fs_descriptor;
	int res = parse_vsector(&fs_descriptor, buffer);
	if(res != 0)
	{
		printf("Failed to located the vsector!");
		return 2;
	}

	close(fd);
	//close(fd);
	return 0;
	

}
