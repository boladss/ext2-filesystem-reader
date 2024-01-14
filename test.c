#include <stdio.h>
#include "parseDir.c"

void test_parseDir(FILE * fs){
    superblock * sb = parseSuperBlock(fs);
    
    printf("block size: %u\n", sb->block_sz);
    printf("number of blocks per group: %u\n", sb->block_num);
    printf("number of inodes per group: %u\n", sb->inode_num);
    printf("BGDT block: %u\n", sb->bgdt_block);
    printf("inode size: %u\n", sb->inode_sz);

    //printf("block num of inode 2: %u\n\n", findBlockNumber(fs, sb, 2));
    uchar inode_buffer[sb->inode_sz+1];
    getInode(fs, sb, 2, inode_buffer);
    inode_buffer[sb->inode_sz] = '\0';
    printf("data of inode 2:\n");

    for(int i = 0; i < sb->inode_sz; i++){
        printf("buffer[%d]: %x\n", i, inode_buffer[i]);
    }
}


int main(){
    FILE * fs = fopen("testfs", "r");

    test_parseDir(fs);

    return 0;
}