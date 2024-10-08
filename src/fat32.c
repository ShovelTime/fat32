#include <assert.h>
#include <uchar.h>
#define FUSE_USE_VERSION 30
#include <fuse3/fuse.h>
#include <fuse3/fuse_lowlevel.h>
#include <string.h>
#include <stdlib.h>
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

const char *reserved_filenames[] = {
    "CON", "PRN", "AUX", "NUL", 
    "COM0", "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9", 
    "COM¹", "COM²", "COM³", 
    "LPT0", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9", 
    "LPT¹", "LPT²", "LPT³"
};

typedef unsigned char char8_t;
size_t current_cluster_number = 2; //lmao imagine using side-effects
char8_t cluster_buffer[SECTOR_SIZE * MAX_SECTOR_PER_CLUSTER];
char16_t LFN_buffer[256]; //buffer to hold lfn names

struct fat32_descriptor {
	char8_t sec_per_cluster; // 0x0D
	unsigned short reserved_sectors_amnt; // 0x0E, default is typically 0x20;
	unsigned int sec_per_fat; //0x24
	unsigned int root_first_cluster; //0x2C //usually at 0x2;
	unsigned long cluster_begin;
	unsigned long bytes_per_cluster;
};

struct fat32_descriptor fs_descriptor;

struct fat32_record {
	char8_t name[11]; 
	char8_t attrib; 
	unsigned long pad1; 
	unsigned short first_clust_high;
	unsigned int pad2;
	unsigned short first_clust_low;
	unsigned int file_size;
}__attribute__((packed));

struct lfn_record {
	char8_t seq_num; // where this record fits within the sequence.
	char8_t name1[10];//first name part
	char8_t attrib; // must be 0x0F
	char8_t dir_type; //must be 0
	char8_t checksum; //how does it work?
	char8_t name2[12]; //second name part
	unsigned short unused; // used to be first_clust_low, MUST be zero
	char8_t name3[4]; // final name part;

}__attribute__((packed));

struct fat32_record* const cluster_records = (struct fat32_record*)cluster_buffer;


// copies wstring from src to dest until null OR n-defined amount of wchar copied is reached
// apparently not find a thing in stdlib, thanks Obama, but I aint doing constant conversions from wchar to char16 and vice-versa
// will NOT include the null terminator!!!
// returns amount of characters copied.
int wscpy_nt(char16_t* dest, const char16_t* src, const size_t len)
{
	int n_copied = 0;	
	while(n_copied < len && *src != u'\0')
	{
		dest[n_copied] = src[n_copied];
		n_copied++;
	}
	return n_copied;
}

//convert a byte into its binary representation in a string.
//PLEASE FOR THE LOVE OF GOD DONT FORGET THE NULL BYTE AT OFFSET 8
void uchar_into_bits_str(char8_t* bit_str, const char8_t byte)
{
	for(int i = 0; i < 8; i++)
	{
		bit_str[7 - i] = ((byte >> i) // move the relevant bit to the first position
			& 1) //then filter out the other bits
			+ 48; //add the ASCII code for '0' to the result, if the bit was set, the sum will be 49, which is the ASCII code for '1'
	}
}

int is_lfn_record(const struct fat32_record* record)
{
	return (record->attrib & 0b00001111) == 0x0F;
}

//convert LFN into SFN representation
//TODO: how is the LFN to SFN conversion for extensions considered?
int lfn_to_sfn(char8_t* dest, const char16_t src, const size_t src_len);

//Null terminated go brrr
void print_buffer(const char8_t* buffer, const size_t len) {
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

//See above but char16_t
void print_wbuffer(const char16_t* buf, const size_t len, const char* suffix)
{
	int i = 0;
	while(i < len)
	{
		printf("%lc", buf[i]);
		if(errno != 0)
		{
			int fail = errno;
			printf("failed to print char16! \n %s \n", strerror(fail));
			break;
		}
		i++;
	}
	printf("%s", suffix);
}

//parse lfn records into LFN_buffer.
//the record pointer should be pointing the record within the buffer itself.
//returns character count in buffer, -1 if there is no lfn assigned to the record.
int get_lfn(const struct fat32_record* record) {
	//Sanity check, make sure it points to a record in the buffer.
	long offset = (long)record - (long)cluster_buffer;
	if(offset > (SECTOR_SIZE * MAX_SECTOR_PER_CLUSTER) || offset < 0)
	{
		printf("invalid pointer to buffer, offset from buffer at %p : %ld", cluster_buffer, offset);
		return -2;
	}

	const struct lfn_record* reader = (struct lfn_record*)record;
	//while()
	return 0;
}

int read_lfn_extension(char16_t);

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
				if(is_lfn_record(&current)){
					printf("LFN RECORD\n");
					char16_t lfn_segment[14] = {L'\0'};
					size_t offset = 0;
					const struct lfn_record* lfn_view = (struct lfn_record*) &current;
					offset += wscpy_nt(lfn_segment, (const char16_t*)lfn_view->name1, 5);
					printf("%ld\n", offset),
					offset += wscpy_nt(lfn_segment + offset + 1, (const char16_t*)lfn_view->name2, 6);
					printf("%ld\n", offset),
					offset += wscpy_nt(lfn_segment + offset + 1, (const char16_t*)lfn_view->name3, 2);
					printf("%ld\n", offset),
					printf("name: ");
					print_wbuffer(lfn_segment, 13, "\n\n");
					break;					

				}

				char8_t attr_bits[9] = {0};
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
size_t compute_sector_cluster(const size_t cluster_number, const struct fat32_descriptor* fs_info) {
	return fs_info->cluster_begin + ((cluster_number - 2) * fs_info->sec_per_cluster);
}


// load cluster into global buffer
int read_into_cluster_buffer(const size_t cluster_number, const int fd)
{

	size_t sector_offset = compute_sector_cluster(cluster_number, &fs_descriptor);
	lseek(fd, sector_offset * SECTOR_SIZE, SEEK_SET);

	//TODO: Investigate edge case of EOF
	int bytes_read = read(fd, cluster_buffer, fs_descriptor.bytes_per_cluster);
	if(errno != 0)
	{
		int fail = errno;
		printf("Failed to read cluster %lu! Reason: %s", cluster_number , strerror(fail));
		exit(fail);
	}
	current_cluster_number = cluster_number;
	return bytes_read;
}


//move buffer into directory.
//returns cluster number on successful move, -1 on failure to find directory name
int move_into_directory(const char* directory_name, const int fd, const size_t name_len)
{
	if(name_len > 11)//LFN name, oh dear
	{
		return -2; // not implemented yet lmao;
	}
	return 0;
}


int parse_vsector(struct fat32_descriptor* container, const char8_t* const sector) {
	unsigned short magic_num = *((short*)(sector + 0x1FE));
	if(magic_num != MAGIC_SIG) {
		printf("Invalid volume sector, wrong signature number!\n");
		return 1; // ma
	}

	if(sector[0x10] != FAT_NUMBER) {
		printf("Invalid volume sector, wrong FAT number!\n");
		return 2;

	}
	if(*((unsigned short*)(sector + 0x0B)) != SECTOR_SIZE)
	{
		printf("Invalid volume sector, wrong Bytes Per Sector!\n");
	 	return 3;
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
	assert(sizeof(struct fat32_record) == 32);	
	assert(sizeof(struct lfn_record) == 32);
	assert(sizeof(char16_t) == 2);
	//printf("Size of record struct : %lu\n", sizeof(struct fat32_record));
	if(argc < 2) {
		printf("E:Device file argument expected. \n");
		return 1;
	}
	const char* path = argv[1];
	struct stat device_info;
	printf("%s \n", path); 
	char8_t init_buffer[SECTOR_SIZE];
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

	lseek(fd, compute_sector_cluster(2, &fs_descriptor) * SECTOR_SIZE, SEEK_SET); // go to begginning of root cluster;
	read(fd, cluster_buffer, fs_descriptor.bytes_per_cluster);
	print_buffer(cluster_buffer, 512 * 2);
	print_records(cluster_records);

	close(fd);
	//close(fd);
	return 0;
	

}



      
