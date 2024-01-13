#include <stdio.h>
#include "parseDir.c"

void test_parseDir(FILE * fs){
    superblock * sb = parseSuperBlock(fs);
    
    printf("block size: %u\n", sb->block_sz);
    printf("number of blocks per group: %u\n", sb->block_num);
    printf("number of inodes per group: %u\n", sb->inode_num);
    printf("BGDT block: %u\n", sb->bgdt_block);
    printf("inode size: %u\n", sb->inode_sz);
    
    uint inode_start = getInodeTableBlock(fs, 0, sb->bgdt_block, sb->block_sz);
    printf("starting block of inode table: %u\n\n", inode_start);

    printf("block group num of inode 2: %u\n", computeBlockGroupNum(sb, 2));
    printf("index of inode 2: %u\n", computeInodeIndex(sb, 2));
    printf("block num of inode 2: %u\n\n", computeInodeBlock(sb, 2, inode_start));
}


int main(){
    FILE * fs = fopen("testfs", "r");

    test_parseDir(fs);

    return 0;
}