#include <stdio.h>
#include <stdlib.h>

typedef unsigned int uint;
typedef unsigned char uchar;

#define SUPERBLOCK_SZ 1024

typedef struct superblock{
    uint block_sz; // byte offsets 24-27
    uint block_num; // byte offsets 32-35
    uint inode_num; // byte offsets 40-43
    uint bgdt_addr; // block after superblock
} superblock;

// add variables to store default values
//size of bgdt = number of blocks * 32


// take in filesystem file and starting superblock block address
uint readInt(FILE *fs, uint addr, uint offset, uint size){
    uchar buffer[size];

    fseek(fs, addr+offset, SEEK_SET);
    fread(buffer, size, 1, fs);

    uint ret = 0;

    //little endian
    for(int i = 0; i < size; i++){
        ret |= buffer[i] << i*4;
        //printf("buffer[%d]: %u\nret: %u\n", i, buffer[i], ret);
    }

    return ret;
}

superblock * parseSuperBlock(FILE *fs){
    superblock * sb = (superblock *) malloc(sizeof(superblock));
    //get block size
    sb->block_sz = SUPERBLOCK_SZ << readInt(fs, SUPERBLOCK_SZ, 24, 4);
    sb->block_num = readInt(fs, SUPERBLOCK_SZ, 32, 4);
    sb->inode_num = readInt(fs, SUPERBLOCK_SZ, 40, 4);
    sb->bgdt_addr = (SUPERBLOCK_SZ / sb->block_sz) + 1;

    return sb;
}