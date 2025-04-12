#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// Credit to wikipedia for the CRC32 algorithm
typedef union {
    uint32_t crc;
    uint8_t c[4];
} crc_32_t;


unsigned char buffer[3000];
FILE *ptr;

const char *default_file = "test.bin";
const unsigned int polynomial = 0x04C11DB7;
static unsigned int CRCTable[256];


unsigned int reverse_bytes(unsigned int crc) {
    unsigned int result = 0;
    for(int i = 0; i < 4; i++) {
        result = (result << 8) | (crc & 0xFF);
        crc >>= 8;
    }
    return result;
}

unsigned int reverse_bits(unsigned int crc) {
    unsigned int result = 0;
    for(int i = 0; i < 32; i++) {
        result = (result << 1) | (crc & 1);
        crc >>= 1;
    }
    return result;
}

// Initialization by multiple threads is redundant, but safe.
static void CRC32_init(void)
{
	unsigned int reversed_poly = reverse_bits(polynomial);

    unsigned int crc32 = 1;
    // C guarantees CRCTable[0] = 0 already.
	for (unsigned int i = 128; i; i >>= 1) {
		crc32 = (crc32 >> 1) ^ (crc32 & 1 ? reversed_poly : 0);
		for (unsigned int j = 0; j < 256; j += 2*i)
        	CRCTable[i + j] = crc32 ^ CRCTable[j];
	}
}


unsigned int CRC32(const unsigned char data[], size_t data_length)
{
	unsigned int crc32 = 0xFFFFFFFFu;

	if (CRCTable[255] == 0)
		CRC32_init();
	
	for (size_t i = 0; i < data_length; i++) {
		crc32 ^= data[i];
		crc32 = (crc32 >> 8) ^ CRCTable[crc32 & 0xff];
	}
	
	// Finalize the CRC-32 value by inverting all the bits
	crc32 ^= 0xFFFFFFFFu;
	return crc32;
}

unsigned int CRC32_cont(const unsigned char data[], size_t data_length, unsigned int crc)
{
    if (CRCTable[255] == 0)
        CRC32_init();
    
    for (size_t i = 0; i < data_length; i++) {
        crc ^= data[i];
        crc = (crc >> 8) ^ CRCTable[crc & 0xff];
    }
    
    return crc;
}

// expected use: crc32 test.bin
int main(int argc, char** argv) {

    //Parse argv
    if(argc != 2) {
        printf("Usage: crc32 <filename>\n");
        printf("number of arguments: %d\n", argc);
        return 1;
    }
    
    const char *filename = argv[1];
    if(filename == NULL) {
        filename = default_file;
        printf("Error: Bad filename provided, using default file: %s\n", filename);
    }


    ptr = fopen(filename,"rb");  // r for read, b for binary
    if (ptr==NULL) {
        printf("Error: File not found\n");
        return 1;
    }
    
    int size = -1;

    crc_32_t crc;
    crc.crc = 0xFFFFFFFF;

    while(size = fread(buffer,1,64,ptr)) {
        printf("Read %d bytes\n", size);
        
        crc.crc = CRC32_cont(buffer, size, crc.crc);
        crc.crc &= 0xFFFFFFFF;
        
    }
    fclose(ptr);

    // crc = reverse_bits(crc);
    crc.crc = (~crc.crc) & 0xFFFFFFFF;
    printf("CRC32: %X\n", crc.crc);

    ptr = fopen(filename,"ab");
    if (ptr==NULL) {
        printf("Error: Cannot write to file\n");
        return 1;
    }
    
    for( int i = 0 ; i < 4 ; i++ ) {
        uint8_t c = crc.c[i];
        fwrite(&c, sizeof(c), 1, ptr);
    }

    printf("Appended CRC32 to %s\n", filename);
    fclose(ptr);


    return 0;
    
}