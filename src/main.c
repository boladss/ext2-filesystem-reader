#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "navigate.c"

// Operation 1 -- Path enumeration
void printAllFiles(char * filename){
    int fs = open(filename, O_RDONLY);
    superblock * sb = parseSuperBlock(fs);
    inode * in = getInode(fs, sb, 2);
    //inode 2 is the root directory
    char path[4096];
    path[0] = '\0';

    parseDirInode(fs, sb, in, path);

    free(in);
    free(sb);

    close(fs);
}

// Operation 2 -- Filesystem object extraction
int copyFiles(char * filesystem, char * filepath){
    int fs = open(filesystem, O_RDONLY);

    int result = navigate(fs, filepath);

    close(fs);

    return result;
}


int main(int argc, char *argv[]){
    if(argc == 2){
        printAllFiles(argv[1]);
        return 0;
    }

    if(argc == 3){
        return copyFiles(argv[1], argv[2]);
    }

    return 0;
}