#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "parseDir.c"

void duplicate_file(int fs, superblock * sb, char * name, inode * in){
    
    // create new file
    int f = open(name, O_CREAT | O_WRONLY);
    uchar data_buf[sb->block_sz];
    uchar curr[1];

    printf("%s\n",name);

    // write to new file
    // go through all data blocks of original file
    //get inode of file
    // for each data block: write to new file

    int bytes_read = 0;

    //direct pointers
    for(int i = 0; i < 12; i++){
        if(in->direct[i] == 0) continue; //skip empty pointers

        getDataBlock(fs, sb, in->direct[i], data_buf);

        int offset = 0;

        while(offset < sb->block_sz && bytes_read < in->file_sz){
            curr[0] = data_buf[offset];
            write(f, curr, 1);
            offset++;
            bytes_read++;
        }
    }

    //indirect pointers

    printf("bytes read: %d\n", bytes_read);
    close(f);

    return;
}