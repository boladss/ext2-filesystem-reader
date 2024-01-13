#include <stdio.h>
#include <stdlib.h>

typedef unsigned int uint;
typedef unsigned char uchar;

#define SB_START 1024

//number of blocks and inodes are wrong
typedef struct superblock{
    uint block_sz; // byte offsets 24-27
    uint block_num; // byte offsets 32-35
    uint inode_num; // byte offsets 40-43
    uint inode_sz; // byte offsets 88-89
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
        ret |= buffer[i] << (i*8);
        //printf("addr: %u\nbuffer[%d]: %u\nret: %u\n\n",SB_START+offset, i, buffer[i], ret);
    }

    return ret;
}

superblock * parseSuperBlock(FILE *fs){
    superblock * sb = (superblock *) malloc(sizeof(superblock));

    //get block size
    sb->block_sz = SB_START << readInt(fs, SB_START+24, 4);
    sb->block_num = readInt(fs, SB_START+32, 4);
    sb->inode_num = readInt(fs, SB_START+40, 4);
    sb->bgdt_block = (SB_START / sb->block_sz) + 1;
    sb->inode_sz = readInt(fs, SB_START+88, 2);

    return sb;
}

uint computeBlockGroupNum(superblock * sb, uint inode){
    return (inode - 1) / sb->inode_num;
}

uint computeInodeIndex(superblock * sb, uint inode){
    return (inode - 1) % sb->inode_num;
}

uint computeInodeBlock(superblock * sb, uint index, uint start){
    return ((index * sb->inode_sz) / sb->block_sz) + start;
}

// returns starting block number of inode table
uint getInodeTableBlock(FILE * fs, uint group_num, uint bgdt_block, uint block_sz){
    uint entry_addr = (bgdt_block*block_sz) + (group_num*32) + 8;

    //printf("entry: %u\n", entry_addr);

    return readInt(fs, entry_addr, 4);
}

// references used: 
// https://wiki.osdev.org/Ext2
// https://www.nongnu.org/ext2-doc/