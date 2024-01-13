#include <stdio.h>
#include <stdlib.h>

typedef unsigned int uint;
typedef unsigned char uchar;

#define BOOT_SZ 1024

typedef struct superblock{
    uint block_sz; // byte offsets 24-27
    uint block_num; // byte offsets 32-35
    uint inode_num; // byte offsets 40-43
    uint inode_sz; // byte offsets 88-89
    uint inode_table; //starting block offset of inode table, should be same for all groups
    uint bgdt_block; // block after superblock
} superblock;

// add variables to store default values

// take in filesystem file and starting superblock block address
uint readInt(FILE *fs, uint offset, uint size){
    uchar buffer[size];

    fseek(fs, offset, SEEK_SET);
    fread(buffer, size, 1, fs);

    uint ret = 0;

    //little endian
    for(int i = 0; i < size; i++){
        ret |= buffer[i] << i*4;
        //printf("addr: %u\nbuffer[%d]: %u\nret: %u\n",BOOT_SZ+offset, i, buffer[i], ret);
    }

    return ret;
}

superblock * parseSuperBlock(FILE *fs){
    superblock * sb = (superblock *) malloc(sizeof(superblock));

    //get block size
    sb->block_sz = BOOT_SZ << readInt(fs, BOOT_SZ+24, 4);
    sb->block_num = readInt(fs, BOOT_SZ+32, 4);
    sb->inode_num = readInt(fs, BOOT_SZ+40, 4);
    sb->bgdt_block = (BOOT_SZ / sb->block_sz) + 1;
    sb->inode_sz = readInt(fs, BOOT_SZ+88, 2);

    uint entry_addr = (sb->bgdt_block*sb->block_sz) + 8;
    sb->inode_table = readInt(fs, entry_addr, 4);

    return sb;
}

uint computeBlockGroupNum(superblock * sb, uint inode){
    return (inode - 1) / sb->inode_num;
}

// references used: 
// https://wiki.osdev.org/Ext2
// https://www.nongnu.org/ext2-doc/