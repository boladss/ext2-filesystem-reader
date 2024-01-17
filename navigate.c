#include <stdio.h>
#include <stdlib.h>
#include "parseDir.c"
#include "filepath.c"


// for operations 2 and 3

// file navigation:
// 1. check root inode
// 2. check root data dir
// 3. check for filename
// 4. repeat checking inode and data until file or directory is found

// file names are separated by forward slashes

// file path < 4096 characteers
// file object name < 256 characters

// takes in FILE pointer and path as inputs
void navigate(FILE * fs, char * path){ 
    superblock * sb = parseSuperBlock(fs); // get superblock info
    inode * in = getInode(fs, sb, 2); // get root inode

    uchar data_buffer[sb->block_sz];

    path = cleanInput(path);
    printf("%s\n", path);

    int i = 1;
    for(int i = 1; i < strlen(path); i++){ // iterates through each filename in path
        char filename[256];

        // copies filename from path
        int j = 0;
        while(i < strlen(path) && path[i] != '/' ){
            filename[j++] = path[i++];
        }
        filename[j] = '\0';


        // check if filename is last in path, account for trailing '/'
        if(i == strlen(path) || path[i] == '/'){
            printf("%s\n", filename);
            printf("end of path\n");
        }

        // if not last : has to be a directory
        // got through each pointer in dir inode
        // for each pointer:
        // look for filename in directory block of current dir inode
        // if filename is found : navigate to filename's inode
        // else : INVALID PATH, return -1

        // search in directory
        //inode direct pointers
        for(int i = 0; i < 12; i++){
            if(in->direct[i] == 0) continue; // skip empty inode
        }

        //inode single indirect

        //inode double indirect

        //inode triple indirect
        
    }

    // using current inode, determine if dir or reg file
    // if dir : copy all contents of dir, return 0
    // if reg file : copy file to host, return 0
}