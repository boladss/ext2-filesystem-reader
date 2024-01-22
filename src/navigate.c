#include <stdio.h>
#include <stdlib.h>
#include "duplicate.c"

// for operation 2

// file navigation:
// 1. check root inode
// 2. check root data dir
// 3. check for filename
// 4. repeat checking inode and data until file or directory is found

// file names are separated by forward slashes

// file path < 4096 characteers
// file object name < 256 characters


// clean filepath input string
// by removing repeated slashes
char * cleanInput(char * input){
    int i = 0;
    char * output = (char*) malloc((4096)*sizeof(char));

    do{
        output[i++] = '/';
        
        // ignore repeated slashes
        while(*input != '\0' &&  *input == '/'){
            input++;
        }

        while(*input != '\0' && *input != '/'){
            output[i++] = *input;
            input++;
        }

    }while(*(input++) != '\0');

    return output;
}


// searches directory data block for file
uint searchBlock(int fs, superblock * sb, inode * in, uint curr_addr, char * filename, uint * bytes_read, uint wantDir){
    uint offset = 0;
    
    while(*bytes_read < in->file_sz && (offset < sb->block_sz)){ 
        dir_entry * dir = getDirEntry(fs, sb, curr_addr+offset);
            
        offset += dir->size;
        *bytes_read += dir->size;

        if(!strcmp(filename, dir->name)){ // file is found
            // get inode and check if directory

            if(wantDir){
                inode * dir_in = getInode(fs, sb, dir->inum);
                if(dir_in->isDir){ // path is found
                    uint result = dir->inum;

                    freeDirEntry(dir);
                    free(dir_in);

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

    return 0; // not found
}


uint searchSingleIndirectDir(int fs, superblock * sb, inode * in, int block_num, char * filename, int * bytes_read, int wantDir){
    uint result;

    // iterate through each pointer
    for(int i = 0; i < sb->block_sz; i+=4){
        uint curr_offset = (block_num*sb->block_sz)+i; // offset of direct pointer
        uint curr_addr = readInt(fs, curr_offset, 4) * 4096; // address of data block

        // skip empty pointers
        if(curr_addr == 0) continue;

        result = searchBlock(fs, sb, in, curr_addr, filename, bytes_read, wantDir);

        if(result != 0) return result;
    }

    return 0; // not found
}


// searches directory and returns inode number of entry found
// wantDir = parameter to indicate if looking for directory
// returns 0 if not found (should be fine since block zero always contains the superblock)
uint searchDir(int fs, superblock * sb, inode * in, char * filename, int wantDir){
    uint bytes_read = 0; // total number of bytes read 
    uint curr_addr;
    uint result;

    // for each direct block
    for(int i = 0; i < 12; i++){
        if(in->direct[i] == 0) continue; // skip empty inode
        
        curr_addr = in->direct[i]*sb->block_sz;

        result = searchBlock(fs, sb, in, curr_addr, filename, &bytes_read, wantDir);

        if(result != 0) return result;
    }

    // single indirect block
    if(in->single_ind){
        result = searchSingleIndirectDir(fs, sb, in, in->single_ind, filename, &bytes_read, wantDir);
        if(result != 0) return result;
    }

    // double indirect block
    if(in->double_ind){ 
        // handler for double indirect block
        // contains pointers to block containing singly indirect blocks

        for(int i = 0; i < sb->block_sz; i+= 4){ // pointer size is 4 bytes
            // get double block

            uint d_block = (in->double_ind*sb->block_sz)+i; // address of direct pointer
            uint d_block_num = readInt(fs, d_block, 4); // data block number

            result = searchSingleIndirectDir(fs, sb, in, d_block_num, filename, &bytes_read, wantDir);
            if(result != 0) return result;
        }
    }

    // triple indirect block
    if(in->triple_ind){
        // handler for triple indirect block
        // contains pointers to blocks containing doubly indirect blocks

        for(int i = 0; i < sb->block_sz; i+= 4){ // pointer size is 4 bytes
            // get triple block
            uint t_block = (in->triple_ind*sb->block_sz)+i;
            uint t_block_num = readInt(fs, t_block, 4); // data block number

            for(int i = 0; i < sb->block_sz; i+= 4){ // pointer size is 4 bytes
                uint d_block = (t_block_num*sb->block_sz)+i; // address of direct pointer
                uint d_block_num = readInt(fs, d_block, 4); // data block number

                result = searchSingleIndirectDir(fs, sb, in, d_block_num, filename, &bytes_read, wantDir);
                if(result != 0) return result;
            }
        }
    }

    return 0; // not found
}

// takes in FILE pointer and path as inputs
int navigate(int fs, char * path){ 
    superblock * sb = parseSuperBlock(fs); // get superblock info
    inode * in = getInode(fs, sb, 2); // get root inode
    int result;

    path = cleanInput(path);

    // if root directory, duplicate everything, no need to search
    if(!strcmp(path, "/")){
        duplicateDir(fs, sb, getInode(fs, sb, 2), "output", "");
        return 0;
    }

    // iterates through each filename in path
    for(int i = 1; i < strlen(path); i++){ 
        char filename[256];

        // copies current filename from path
        int j = 0;
        while(i < strlen(path) && path[i] != '/' ){
            filename[j++] = path[i++];
        }
        filename[j] = '\0';

        // check if filename is last in path i.e., the file/dir we want to extract
        // (accounts for trailing '/')
        if(i == strlen(path) || (i == strlen(path) -1 && path[i] == '/')){
            result = searchDir(fs, sb, in, filename, 0);

            if(result  == 0){
                fprintf(stderr, "INVALID PATH\n");
                return -1;
            }
            else{
                inode * file_in = getInode(fs, sb, result);

                // extract directory
                if(file_in->isDir){
                    duplicateDir(fs, sb, file_in, filename, "output");
                    return 0;
                }
                // extract regular file
                else{
                    duplicateFile(fs, sb, file_in, filename);
                    return 0;
                }
            }

            return 0;
        }

        // not yet the last filename in path?
        // we have to traverse filepath further,
        // obtain next directory/file in the filepath
        result = searchDir(fs, sb, in, filename, 1);

        if(result == 0) {
            fprintf(stderr, "INVALID PATH\n");
            return -1;
        }
        else{
            free(in);
            in = getInode(fs, sb, result);
        }
        
    }
}