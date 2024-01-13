#include <stdio.h>
#include "parseDir.c"

void test_parseDir(FILE * fs){
    superblock * sb = parseSuperBlock(fs);
    
    printf("block size: %u\n", sb->block_sz);
    printf("number of blocks: %u\n", sb->block_num);
    printf("number of inodes: %u\n", sb->inode_num);
    printf("BGDT block: %u\n", sb->bgdt_block);
    printf("inode size: %u\n", sb->inode_sz);
    printf("starting block of inode table: %u\n", getInodeTableBlock(fs, 0, sb->bgdt_block, sb->block_sz));
}

int main(){
    FILE * fs = fopen("testfs", "r");

    test_parseDir(fs);

    return 0;
}