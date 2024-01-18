#include <stdio.h>
#include <stdlib.h>
#include "filepath.c"


// for operations 2

// things left to do:
//   1. indirect handling
//   2. copyinig of file
//   3. return 0 or -1 depending on outcome

// file navigation:
// 1. check root inode
// 2. check root data dir
// 3. check for filename
// 4. repeat checking inode and data until file or directory is found

// file names are separated by forward slashes

// file path < 4096 characteers
// file object name < 256 characters


// searches directory data block for file
int searchBlock(int fs, superblock * sb, inode * in, int curr_addr, char * filename, int * bytes_read, int wantDir){
    int offset = 0;
    
    while(*bytes_read < in->file_sz && (offset < sb->block_sz)){ 
        dir_entry * dir = getDirEntry(fs, sb, curr_addr+offset);
            
        offset += dir->size;
        *bytes_read += dir->size;

        //printf("dir name: %s\n", dir->name);
        //printf("curr adrr: %d\n", curr_addr);

        if(!strcmp(filename, dir->name)){ // file is found
                // get inode and check if directory

            if(wantDir){
                inode * dir_in = getInode(fs, sb, dir->inum);
                if(dir_in->isDir){ // path is found
                    int result = dir->inum;

                    freeDirEntry(dir);
                    free(dir_in);

                    return result;
                    }
                free(dir_in);
            }
            else{
                int result = dir->inum;
                freeDirEntry(dir);
                return result;
            }
        }

        freeDirEntry(dir);
    }

    return 0;
}

// searches directory and returns inode number of entry found
// additionally parameter to indicate if looking for directory
// returns 0 if not found (should be fine since block zero always contains the superblock)
int searchDir(int fs, superblock * sb, inode * in, char * filename, int wantDir){
    // search in directory
    //inode direct pointers

    int bytes_read = 0; // total number of bytes read 
    int offset = 0;
    int curr_addr;
    int result;

    // for each direct block
    for(int i = 0; i < 12; i++){
        if(in->direct[i] == 0) continue; // skip empty inode

        //printf("direct block\n");
        
        curr_addr = in->direct[i]*sb->block_sz;

        result = searchBlock(fs, sb, in, curr_addr, filename, &bytes_read, wantDir);

        if(result != 0) return result;

        /*while(bytes_read < in->file_sz && (offset < sb->block_sz)){ 
            dir_entry * dir = getDirEntry(fs, sb, curr_addr+offset);
            
            offset += dir->size;
            bytes_read += dir->size;

            //printf("dir name: %s\n", dir->name);
            //printf("curr adrr: %d\n", curr_addr);

            if(!strcmp(filename, dir->name)){ // file is found
                // get inode and check if directory

                if(wantDir){
                    inode * dir_in = getInode(fs, sb, dir->inum);
                    if(dir_in->isDir){ // path is found
                        int result = dir->inum;

                        freeDirEntry(dir);
                        free(dir_in);

                        return result;
                    }
                    free(dir_in);
                }
                else{
                    int result = dir->inum;
                    freeDirEntry(dir);
                    return result;
                }
            }

            freeDirEntry(dir);
        }*/
    }

    //inode single indirect
    if(in->single_ind){
        //printf("single\n");

        //iterate through each pointer
        for(int i = 0; i < sb->block_sz; i+=4){
            int curr_offset = (in->single_ind*sb->block_sz)+i; // offset of direct pointer
            int curr_addr = readInt(fs, curr_offset, 4) * 4096; // address of data block

            //skip empty pointers
            if(curr_addr == 0) continue;

            printf("%x\n", curr_addr);

            result = searchBlock(fs, sb, in, curr_addr, filename, &bytes_read, wantDir);
            printf("%d\n", result);

            if(result != 0) return result;
        }
    }

    //inode double indirect

    //inode triple indirect

    return 0;
}

// takes in FILE pointer and path as inputs
void navigate(int fs, char * path){ 
    superblock * sb = parseSuperBlock(fs); // get superblock info
    inode * in = getInode(fs, sb, 2); // get root inode
    int result;

    uchar data_buffer[sb->block_sz];

    path = cleanInput(path);
    printf("%s\n", path);

    // if root directory
    if(!strcmp(path, "/")){
        printf("root po\n\n");
        //printf("inode addr: %x\n", in->addr);
        return;
    }

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

            result = searchDir(fs, sb, in, filename, 0);

            if(result  == 0){
                printf("no such file ://\n");
            }
            else{
                inode * in_2 = getInode(fs, sb, result);
                //printf("inode addr: %x\n", in_2->addr);

                //if directory
                if(in_2->isDir){
                    printf("directory desu <3\n");
                }
                // regular file
                else{
                    printf("file da yo :3\n");
                }
            }

            printf("\n");
            return;
        }

        result = searchDir(fs, sb, in, filename, 1);

        if(result == 0) {
            printf("no file found\n");
            return;
        }
        else{
            free(in);
            in = getInode(fs, sb, result);
        }
        
    }
}