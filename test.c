#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "navigate.c"


void printAllFiles(char * filename){
    int fs = open(filename, O_RDONLY);
    superblock * sb = parseSuperBlock(fs);
    inode * in = getInode(fs, sb, 2);
    char path[4096];
    path[0] = '\0';

    parseDirInode(fs, sb, in, path);

    free(in);
    free(sb);

    close(fs);
}

int copyFiles(char * filepath){
    char * clean_filepath = cleanInput(filepath);
    int fs = open("testfs", O_RDONLY);

    return navigate(fs, filepath);
}


int main(int argc, char *argv[]){
    if(argc == 2){
        printAllFiles(argv[1]);
        return 0;
    }

    if(argc == 3){
        return copyFiles(argv[2]);
    }

    return 0;
}