#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

// TODO: add error handling
// implementation based on file name, will probably have to modify
// to use inodes and reading the actual filesystem

void duplicate_file(char * name){
    int f = open(name, O_RDONLY);

    // create new file

    printf("%s\n",name);

    int * new_f = open(name, O_WRONLY | O_CREAT);

    // write to new file
    // go through all data blocks of original file

    //get inode of file
    // for each data block: write to new file

    close(f);
    close(new_f);
}