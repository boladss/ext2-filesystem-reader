#include <stdio.h>
#include "parseDir.c"

void test_parseDir(FILE * fs){
    superblock * sb = parseSuperBlock(fs);
    
    printf("block size: %u\n", sb->block_sz);
    printf("number of blocks per group: %u\n", sb->num_of_blocks);
    printf("number of inodes per group: %u\n", sb->num_of_inodes);
    printf("BGDT block: %u\n", sb->bgdt_block_num);
    printf("inode size: %u\n\n", sb->inode_sz);

    //printf("block num of inode 2: %u\n\n", findBlockNumber(fs, sb, 2));
    uchar inode_buffer[sb->inode_sz];
    inode * in = getInode(fs, sb, 2);
    /*printf("data of inode 2:\n");

    for(int i = 0; i < 16; i++){
        for(int j = 0; j < 16; j++){
            printf("%02x ", inode_buffer[(i*16)+j]);
        }
        printf("\n");
    }

    printf("\n");*/

    uchar data_buffer[sb->block_sz];
    getDataBlock(fs, sb, 1537, data_buffer);

    //for(int i = 0; i < sb->block_sz; i++){
    //    printf("%c", data_buffer[i]);
    //}

    char path[4096];

    parseDirInode(fs, sb, in, path);

    free(in);
}

void printAllFiles(FILE * fs){
    superblock * sb = parseSuperBlock(fs);
    inode * in = getInode(fs, sb, 2);
    char path[4096];
    path[0] = '\0';

    parseDirInode(fs, sb, in, path);

    free(in);
}


int main(int argc, char *argv[]){
    FILE * fs = fopen(argv[1], "r");

    
    printf("args: %d\n", argc);

    if(argc == 2){
        printAllFiles(fs);
    }

    
    fclose(fs);

    return 0;
}