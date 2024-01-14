#include <stdio.h>
#include <stdlib.h>

typedef unsigned int uint;
typedef unsigned char uchar;

#define SB_START 1024

typedef struct superblock{
    uint block_sz; // byte offsets 24-27
    uint block_num; // byte offsets 32-35
    uint inode_num; // byte offsets 40-43
    uint inode_sz; // byte offsets 88-89
    uint bgdt_block; // block after superblock
} superblock;

// add variables to store default values ?

// reads file and returns an unsigned int based on data read
uint readInt(FILE *fs, uint offset, uint size){
    uchar buffer[size];
    uint ret = 0;

    fseek(fs, offset, SEEK_SET);
    fread(buffer, size, 1, fs);

    //little endian
    for(int i = 0; i < size; i++){
        ret |= buffer[i] << (i*8);
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

// get inode data using inode number
void getInode(FILE * fs, superblock * sb, uint inode, uchar * buffer){
    uint block_group_num = (inode - 1) / sb->inode_num; // which block group the inode is in

    uint entry_addr = (sb->bgdt_block*sb->block_sz) + (block_group_num*32) + 8;
    uint inode_table_start = readInt(fs, entry_addr, 4); // starting block of inode table

    uint inode_index = (inode - 1) % sb->inode_num; // index of inode in table
    uint inode_block_number = ((inode_index * sb->inode_sz) / sb->block_sz) + inode_table_start; // block containing inode
    uint inode_addr = (inode_block_number * sb->block_sz) + (inode_index*sb->inode_sz); // addr of inode entry

    fseek(fs, inode_addr, SEEK_SET);
    fread(buffer, sb->inode_sz, 1, fs);

    return;
}

// get data block using block number
void getDataBlock(FILE * fs, superblock * sb, uint block, uchar * buffer){
    uint addr = sb->block_sz * block;

    fseek(fs, addr, SEEK_SET);
    fread(buffer, sb->block_sz, 1, fs);

    return;
}


// references used: 
// https://wiki.osdev.org/Ext2
// https://www.nongnu.org/ext2-doc/