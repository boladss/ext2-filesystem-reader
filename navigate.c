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

// searches directory and returns inode number of entry found
// 
// returns -1 if not found
int searchDir(FILE * fs, superblock * sb, inode * in, char * filename, int wantDir){
    // search in directory
    //inode direct pointers

    int bytes_read = 0; // total number of bytes read 
    int offset = 0;
    int curr_addr;

    // for each direct block
    for(int i = 0; i < 12; i++){
        if(in->direct[i] == 0) continue; // skip empty inode
        
        offset = 0;
        curr_addr = in->direct[i]*sb->block_sz;

        while(bytes_read < in->file_sz && (offset < sb->block_sz)){ 
            dir_entry * dir = getDirEntry(fs, sb, curr_addr+offset);
            
            offset += dir->size;
            bytes_read += dir->size;

            //printf("dir name: %s\n", dir->name);
            //printf("curr adrr: %d\n", curr_addr);

            if(!strcmp(filename, dir->name)){ // file is found
                // get inode and check if directory
                //printf("hewo\n");
                if(wantDir){
                    inode * dir_in = getInode(fs, sb, dir->inum);
                    if(dir_in->isDir){ // path is found
                        uint result = dir->inum;

                        freeDirEntry(dir);
                        free(dir_in);

                        //printf("path found\n");

                        return result;
                    }
                    free(dir_in);
                }
                else{
                    uint result = dir->inum;
                    freeDirEntry(dir);
                    return result;
                }
            }

            freeDirEntry(dir);
        }
    }

    //inode single indirect

    //inode double indirect

    //inode triple indirect

    return -1;
}

// takes in FILE pointer and path as inputs
void navigate(FILE * fs, char * path){ 
    superblock * sb = parseSuperBlock(fs); // get superblock info
    inode * in = getInode(fs, sb, 2); // get root inode
    int result;

    uchar data_buffer[sb->block_sz];

    path = cleanInput(path);
    printf("%s\n", path);

    for(int i = 1; i < strlen(path); i++){ // iterates through each filename in path
        char filename[256];

        // copies filename from path
        int j = 0;
        while(i < strlen(path) && path[i] != '/' ){
            filename[j++] = path[i++];
        }
        filename[j] = '\0';

        printf("%s\n", filename);

        // check if filename is last in path, account for trailing '/'
        if(i == strlen(path) || (i == strlen(path) -1 && path[i] == '/')){
            printf("end of path\n");
            printf("last file: %s\n", filename);

            // if file is invalid
            result = searchDir(fs, sb, in, filename, 0);

            if(result  == -1){
                printf("no such file ://\n");
            }
            else{
                inode * in_2 = getInode(fs, sb, result);

                //if directory
                if(in_2->isDir){
                    printf("directory desu <3\n");
                }
                // regular file
                else{
                    printf("file da yo :3\n");
                }
            }

            // if file is regular file

            printf("\n");
            return;
        }

        // if not last : has to be a directory
        // got through each pointer in dir inode
        // for each pointer:
        // look for filename in directory block of current dir inode
        // if filename is found : navigate to filename's inode
        // else : INVALID PATH, return -1

        result = searchDir(fs, sb, in, filename, 1);

        //printf("next inode: %d\n", result);

        if(result == -1) {
            printf("no file found\n");
            return;
        }
        else{
            free(in);
            in = getInode(fs, sb, result);
        }
        
    }

    // using current inode, determine if dir or reg file
    // if dir : copy all contents of dir, return 0
    // if reg file : copy file to host, return 0
}