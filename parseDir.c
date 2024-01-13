#include <stdio.h>
typedef unsigned int uint;
typedef unsigned char uchar;


// take in filesystem file and starting superblock block address

uint getBlockSize(FILE *fs, uint addr){
    uchar buffer[1052];

    printf("addr: %p\n", fs);
    printf("addr: %p\n", fs+addr);
    printf("addr: %p\n", fs+addr+24);

    fread(buffer, 1052, 1, fs);

    uint size = 0;

    for(int i = 1048; i < 1052; i++){
        printf("buffer[%d]: %u\n", i, buffer[i]);
        size |= buffer[i] << (3-i);
    }

    return size;
}

void parseSuperBlock(FILE *fs, uint addr){
    uint block_sz; // bytes 24-27
    uint block_num; 
    uint inode_num;
    uint bgdt_addr;

    //get block size
    block_sz = getBlockSize(fs, addr);

    printf("block size: %u\n", block_sz);
}