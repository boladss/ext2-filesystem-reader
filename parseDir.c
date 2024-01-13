#include <stdio.h>
typedef unsigned int uint;
typedef unsigned char uchar;

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

void parseSuperBlock(FILE *fs, uint addr){
    uint block_sz; // byte offsets 24-27
    uint block_num; // byte offsets 32-35
    uint inode_num; // byte offsets 40-43
    uint bgdt_addr; // block after superblock

    //get block size
    block_sz = 1024 << readInt(fs, addr, 24, 4);
    block_num = readInt(fs, addr, 32, 4);
    inode_num = readInt(fs, addr, 40, 4);
    bgdt_addr = 2;


    printf("block size: %u\n", block_sz);
    printf("number of blocks: %u\n", block_num);
    printf("number of inodes: %u\n", inode_num);
    printf("BGDT address: %u\n", bgdt_addr);
}