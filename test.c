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
    uchar inode_buffer[sb->inode_sz];
    getInode(fs, sb, 2, inode_buffer);
    printf("data of inode 2:\n");

    for(int i = 0; i < 16; i++){
        for(int j = 0; j < 16; j++){
            printf("%02x ", inode_buffer[(i*16)+j]);
        }
        printf("\n");
    }
}


int main(){
    FILE * fs = fopen("testfs", "r");

    test_parseDir(fs);

    fclose(fs);

    return 0;
}