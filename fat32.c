#define FUSE_USE_VERSION 30
#include <fuse3/fuse.h>
#include <fuse3/fuse_lowlevel.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>

#define SECTOR_SIZE 512 // true across every fat32 filesystem, if this changes something is VERY wrong;
#define MAX_SECTOR_PER_CLUSTER 128
#define FAT_NUMBER 2 // number of lookup tables in the filesystem, always 2 according to spec.
#define MAGIC_SIG 0xAA55 // Magic number identifying the volume ID sector. Offset at 0x1FE in vsector.
#define RECORD_PER_SECTOR 16 //512 byte sectors can hold 16 32-bytes records.

unsigned long true_sector_count = 0; //amount of data that will actually be read into the buffer
unsigned char cluster_buffer[SECTOR_SIZE * MAX_SECTOR_PER_CLUSTER];

struct fat32_descriptor {
	unsigned char sec_per_cluster; // 0x0D
	unsigned short reserved_sectors_amnt; // 0x0E, default is typically 0x20;
	unsigned int sec_per_fat; //0x24
	unsigned int root_first_cluster; //0x2C //usually at 0x2;
	unsigned long cluster_begin;
	unsigned long bytes_per_cluster;
};

struct fat32_descriptor fs_descriptor;

struct fat32_record {
	unsigned char name[11]; 
	unsigned char attrib; 
	unsigned long pad1; 
	unsigned short first_clust_high;
	unsigned int pad2;
	unsigned short first_clust_low;
	unsigned int file_size;
}__attribute__((packed));

struct fat32_record* const cluster_records = (struct fat32_record*)cluster_buffer;


void uchar_into_bits_str(unsigned char* bit_str, const unsigned char byte)
{
	for(int i = 0; i < 8; i++)
	{
		bit_str[7 - i] = ((byte >> i) // move the relevant bit to the first position
			& 1) //then filter out the other bits
			+ 48; //add the ASCII code for '0' to the result, if the bit was set, the sum will be 49, which is the ASCII code for '1'
	}
}


//Null terminated go brrr
void print_buffer(const unsigned char* buffer, const size_t len) {
	for(int i = 0; i < len; i++)
	{
		if(isalpha(buffer[i]))
		{
			printf("%c", buffer[i]);
		}
		else
     		{
			printf("%u", buffer[i]);
		}

	}
	printf("\n");
};

void print_records(const struct fat32_record* records) {
	for(int i = 0; i < RECORD_PER_SECTOR * fs_descriptor.sec_per_cluster; i++)
	{
		const struct fat32_record current = records[i];
		printf("Record %u\n", i);
		switch (current.name[0]) {
			case 0xE5:
				printf("UNUSED RECORD\n\n");
				break;
			case 0x00:
				printf("END OF DIRECTORY \n\n");
				return;
			default:
				if((current.attrib & 0b00001111) == 0x0F){
					printf("LFN RECORD\n");
					printf("Filename cluster: %u\n\n", (current.first_clust_high << 16) + current.first_clust_low);
					break;

				}

				unsigned char attr_bits[9] = {0};
				uchar_into_bits_str(attr_bits, current.attrib);

				
				printf("name: %.11s\n attribute byte: %s\n first cluster hi : %u | lo: %u \n actual cluster number : %u \nfile size: %u\n\n",
	 				current.name,
	 				attr_bits,
					current.first_clust_high,
	 				current.first_clust_low,
					(current.first_clust_high << 16) + current.first_clust_low,
	 				current.file_size	
				);

		}

	}
}

// Compute offset of a cluster down to its sector.
size_t compute_cluster_offset(const unsigned int cluster_number, const struct fat32_descriptor* fs_info) {
	return fs_info->cluster_begin + ((cluster_number - 2) * fs_info->sec_per_cluster);
}

//convert a byte into its binary representation in a string.
//PLEASE FOR THE LOVE OF GOD DONT FORGET THE NULL BYTE AT OFFSET 8

int read_cluster(const size_t cluster_number, unsigned char* const buffer, int fd)
{

	return 0;
}


int parse_vsector(struct fat32_descriptor* container, const unsigned char* const sector) {
	unsigned short magic_num = *((short*)(sector + 0x1FE));
	if(magic_num != MAGIC_SIG) {
		printf("Invalid volume sector, wrong signature number!\n");
		return 1; // ma
	}

	if(sector[0x10] != FAT_NUMBER) {
		printf("Invalid volume sector, wrong FAT number!\n");
		return 2;

	}
	container->sec_per_cluster = sector[0x0D];
	container->reserved_sectors_amnt = *((unsigned short*)(sector + 0x0E));
	container->sec_per_fat = *((unsigned int*)(sector + 0x24));
	container->root_first_cluster = *((unsigned int*)(sector + 0x2C));
	container->cluster_begin = container->reserved_sectors_amnt + (FAT_NUMBER * container->sec_per_fat);
	container->bytes_per_cluster = SECTOR_SIZE * container->sec_per_cluster;
	return 0;
}


int fat32_getattr(const char* path, struct stat* inode_info, struct fuse_file_info *fi) {
	return -1;
}
	


//MAIN
int main(int argc, char* argv[]) {
	if(argc < 2) {
		printf("E:Device file argument expected. \n");
		return 1;
	}
	const char* path = argv[1];
	struct stat device_info;
	printf("%s \n", path); 
	unsigned char init_buffer[SECTOR_SIZE];
	int fd = open(path, O_RDONLY, O_SYNC);
	if (errno != 0) {
		int fail = errno;
		printf("Failed to open device! error: %s \n", strerror(fail));
		return fail;
	}
	stat(path, &device_info);
	if (errno != 0) {
		int fail = errno;
		printf("Failed to open stat! error: %s \n", strerror(fail));
		return fail;
	}
	read(fd, init_buffer, SECTOR_SIZE);
	print_buffer(init_buffer, SECTOR_SIZE);	

	int res = parse_vsector(&fs_descriptor, init_buffer);
	if(res != 0) {
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
	read(fd, cluster_buffer, fs_descriptor.bytes_per_cluster);
	print_buffer(cluster_buffer, 512 * 2);
	print_records(cluster_records);

	close(fd);
	//close(fd);
	return 0;
	

}



      
