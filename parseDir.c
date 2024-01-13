#include <stdio.h>
typedef unsigned int uint;
typedef unsigned char uchar;


// take in filesystem file and starting superblock block address

uint readInt(FILE *fs, uint addr, uint offset, uint size){
    uchar buffer[size];

    fseek(fs, addr+offset, SEEK_SET);
    fread(buffer, size, 1, fs);

    uint ret = 0;

    for(int i = 0; i < size; i++){
        ret |= buffer[i] << i*4;
        printf("buffer[%d]: %u\nret: %u\n", i, buffer[i], ret);
    }

    return ret;
}

void parseSuperBlock(FILE *fs, uint addr){
    uint block_sz; // bytes 24-27
    uint block_num; 
    uint inode_num;
    uint bgdt_addr;

    //get block size
    block_sz = 1024 << readInt(fs, addr, 24, 4);

    printf("block size: %u\n", block_sz);
}