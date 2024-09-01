#define FUSE_USE_VERSION 30
#include <fuse3/fuse.h>
#include <fuse3/fuse_lowlevel.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

const unsigned short SECTOR_SIZE = 512; // true across every fat32 filesystem, if this changes something is VERY wrong;
const unsigned char FAT_NUMBER = 2; // What does that even mean?
const unsigned short MAGIC_SIG = 0xAA55; // Magic number identifying the volume ID sector. Offset at 0x1FE in vsector.

struct fat32_descriptor
{
	unsigned char sec_per_cluster; // 0x0D
	unsigned short reserved_sectors_amnt; // 0x0E, default is typically 0x20;
	unsigned int sec_per_fat; //0x24
	unsigned int root_first_cluster; //0x2C //usually at 0x2;
	unsigned long cluster_begin;
	
};

// Compute offset of a cluster down to its sector.
size_t compute_cluster_offset(const unsigned int cluster_number, const struct fat32_descriptor* fs_info)
{
	return fs_info->cluster_begin + ((cluster_number - 2) * fs_info->sec_per_cluster);
}

int parse_vsector(struct fat32_descriptor* container, const unsigned char* const sector)
{
	unsigned short magic_num = *((short*)(sector + 0x1FE));
	if(magic_num != MAGIC_SIG)
	{
		printf("Invalid volume sector, wrong signature number!\n");
		return 1; // ma
	}

	if(sector[0x10] != FAT_NUMBER)
	{
		printf("Invalid volume sector, wrong FAT number!\n");
		return 2;

	}
	container->sec_per_cluster = sector[0x0D];
	container->reserved_sectors_amnt = *((unsigned short*)(sector + 0x0E));
	container->sec_per_fat = *((unsigned int*)(sector + 0x24));
	container->root_first_cluster = *((unsigned int*)(sector + 0x2C));
	container->cluster_begin = container->reserved_sectors_amnt + (2 * container->sec_per_fat);
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
	struct stat device_info;
	printf("%s \n", path); 
	unsigned char buffer[512];
	int fd = open(path, O_RDONLY, O_SYNC);
	if (errno != 0)
	{
		int fail = errno;
		printf("Failed to open device! error code is %d \n", fail);
		return fail;
	}
	stat(path, &device_info);
	if (errno != 0)
	{
		int fail = errno;
		printf("Failed to open stat! error code is %d \n", fail);
		return fail;
	}
	read(fd, buffer, 512);
	

	struct fat32_descriptor fs_descriptor;
	int res = parse_vsector(&fs_descriptor, buffer);
	if(res != 0)
	{
		printf("Failed to parse the vsector!");
		close(fd);
		return res;
	}

	printf("Successfully parsed vsector.\n Sectors per cluster: %u\n Reserved sector count: %u\n Sectors per FAT: %u\n Root first cluster: %u\n Cluster begin sector: %lu \n",
	fs_descriptor.sec_per_cluster,
	fs_descriptor.reserved_sectors_amnt,
	fs_descriptor.sec_per_fat,
	fs_descriptor.root_first_cluster,
	fs_descriptor.cluster_begin);

	lseek(fd, compute_cluster_offset(2, &fs_descriptor) * SECTOR_SIZE, SEEK_SET); // go to begginning of root cluster;
	read(fd, buffer, 512);
	printf("%.512s\n", buffer);

	close(fd);
	//close(fd);
	return 0;
	

}



int fat32_getattr(const char* path, struct stat* inode_info, struct fuse_file_info *fi)
{

}
	      
