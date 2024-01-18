#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "parseDir.c"

// copy data block from fs to f
void copyBlock(int fs, int f, superblock * sb, inode * in, int block_num, int * bytes_read){
    uchar data_buf[sb->block_sz];
    uchar curr[1];

    getDataBlock(fs, sb, block_num, data_buf);
    int offset = 0;

    while(offset < sb->block_sz && *bytes_read < in->file_sz){
        curr[0] = data_buf[offset];
        write(f, curr, 1);
        offset++;
        (*bytes_read)++;
    }
}


void copySingleIndBlock(int fs, int f, superblock * sb, inode * in, int block_num, int * bytes_read){
    for(int i = 0; i < sb->block_sz; i+=4){
        int curr_offset = (block_num*sb->block_sz)+i; // address of direct pointer
        int block_num = readInt(fs, curr_offset, 4); // data block number
        
        if(block_num == 0) continue;

        copyBlock(fs, f, sb, in, block_num, bytes_read);
    }
}


void duplicateFile(int fs, superblock * sb, inode * in, char * name){
    
    // create new file
    int f = open(name, O_CREAT | O_WRONLY);

    printf("%s\n",name);

    // write to new file
    // go through all data blocks of original file
    //get inode of file
    // for each data block: write to new file

    int bytes_read = 0;

    //direct pointers
    for(int i = 0; i < 12; i++){
        if(in->direct[i] == 0) continue; //skip empty pointers

        copyBlock(fs, f, sb, in, in->direct[i], &bytes_read);
    }

    //indirect pointers
    //single
    if(in->single_ind){
        copySingleIndBlock(fs, f, sb, in, in->single_ind, &bytes_read);
    }

    //double
    if(in->double_ind){ 
        // handler for double indirect block
        // contains pointers to block containing singly indirect blocks

        for(int i = 0; i < sb->block_sz; i+= 4){ // pointer size is 4 bytes
            // get double block

            int d_block = (in->double_ind*sb->block_sz)+i; // address of direct pointer
            int d_block_num = readInt(fs, d_block, 4); // data block number

           copySingleIndBlock(fs, f, sb, in, d_block_num, &bytes_read);
        }
    }

    //triple
    if(in->triple_ind){
        // handler for triple indirect block
        // contains pointers to blocks containing doubly indirect blocks

        for(int i = 0; i < sb->block_sz; i+= 4){ // pointer size is 4 bytes
            // get triple block
            int t_block = (in->triple_ind*sb->block_sz)+i;
            int t_block_num = readInt(fs, t_block, 4); // data block number

            for(int i = 0; i < sb->block_sz; i+= 4){ // pointer size is 4 bytes
                int d_block = (t_block_num*sb->block_sz)+i; // address of direct pointer
                int d_block_num = readInt(fs, d_block, 4); // data block number

                copySingleIndBlock(fs, f, sb, in, d_block_num, &bytes_read);
            }
        }
    }

    printf("bytes read: %d\n", bytes_read);
    close(f);

    return;
}