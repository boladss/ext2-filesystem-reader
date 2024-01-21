#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "parseDir.c"

// copy data block from fs to f
void copyBlock(int fs, int f, superblock * sb, inode * in, uint block_num, uint * bytes_read){
    uchar data_buf[sb->block_sz];
    getDataBlock(fs, sb, block_num, data_buf);

    uint bytes_to_write;

    // if file size left >= 4096
    // copy block
    if(in->file_sz - *bytes_read >= sb->block_sz){
        bytes_to_write = sb->block_sz;
    }
    else{
        bytes_to_write = in->file_sz - *bytes_read;
    }
    
    write(f, data_buf, bytes_to_write);
    (*bytes_read) += bytes_to_write;
}

void copySingleIndBlock(int fs, int f, superblock * sb, inode * in, uint block_num, uint * bytes_read){
    for(int i = 0; i < sb->block_sz; i+=4){
        uint curr_offset = (block_num*sb->block_sz)+i; // address of direct pointer
        uint block_num = readInt(fs, curr_offset, 4); // data block number
        
        if(block_num == 0) continue;

        copyBlock(fs, f, sb, in, block_num, bytes_read);
    }
}

void duplicateFile(int fs, superblock * sb, inode * in, char * name){
    // create new file
    int f = open(name, O_CREAT | O_WRONLY, 0777);

    // write to new file
    // go through all data blocks of original file
    //get inode of file
    // for each data block: write to new file

    uint bytes_read = 0;

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

            uint d_block = (in->double_ind*sb->block_sz)+i; // address of direct pointer
            uint d_block_num = readInt(fs, d_block, 4); // data block number

           copySingleIndBlock(fs, f, sb, in, d_block_num, &bytes_read);
        }
    }

    //triple
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

                copySingleIndBlock(fs, f, sb, in, d_block_num, &bytes_read);
            }
        }
    }

    close(f);

    return;
}

void duplicateDir(int, superblock *, inode *, char *, char *);

// helper for duplicate dir to copy directory entry blocks
void copyDir(int fs, superblock * sb, inode * in, int block_num, int * bytes_read, char * new_path){
    uint curr_addr = block_num*sb->block_sz;
    uint offset = 0;

        // directory entry
    while(*bytes_read < in->file_sz && (offset < sb->block_sz)){ 
        dir_entry * dir = getDirEntry(fs, sb, curr_addr+offset);
                
        offset += dir->size;
        *bytes_read += dir->size;

        inode * in_2 = getInode(fs, sb, dir->inum);

        // if file, duplicate to output
        if(!in_2->isDir){
            char new_file[4096];
            new_file[0] = '\0';
            strcat(new_file, new_path);
            strcat(new_file, dir->name);

            duplicateFile(fs, sb, in_2, new_file);
        }
        // if directory: recurse
        else if(strcmp(dir->name, ".") && strcmp(dir->name, "..")){ // exclude parent and own directory
            duplicateDir(fs, sb, in_2, dir->name, new_path);
        }

        free(in_2);

        freeDirEntry(dir);
    }
}

void copySingleIndDir(int fs, superblock * sb, inode * in, int block_num, int * bytes_read, char * new_path){
    for(int i = 0; i < sb->block_sz; i+=4){
        uint curr_offset = (block_num*sb->block_sz)+i;
        uint curr_block = readInt(fs, curr_offset, 4);

        //skip empty pointers
        if(curr_block == 0) continue;

        copyDir(fs, sb, in, curr_block, bytes_read, new_path);
    }
}

// duplicate files inside of directory
void duplicateDir(int fs, superblock * sb, inode * in, char * dir_name, char * path){
    // using directory inode
    // traverse through directory and copy each file
    // recurse when next entry in dir is also a directory

    // create output directory

    char new_path[4096];
    new_path[0] = '\0';
    strcat(new_path, path);

    strcat(new_path, dir_name);
    
    mkdir(new_path, 0777);

    strcat(new_path, "/");

    uint bytes_read = 0;

    // traverse through each entry 
    //     if file: duplicate file
    //     if directory: recurse

    // direct pointers
    for(int i = 0; i < 12; i++){
        if(in->direct[i] == 0) continue; // skip empty inode

        copyDir(fs, sb, in, in->direct[i], &bytes_read, new_path);   
    }

    // indirect pointers
    // single
    if(in->single_ind){
        copySingleIndDir(fs, sb, in, in->single_ind, &bytes_read, new_path);
    }

    //double
    if(in->double_ind){ 
        // handler for double indirect block
        // contains pointers to block containing singly indirect blocks

        for(int i = 0; i < sb->block_sz; i+= 4){ // pointer size is 4 bytes
            // get double block

            uint d_block = (in->double_ind*sb->block_sz)+i; // address of direct pointer
            uint d_block_num = readInt(fs, d_block, 4); // data block number

           copySingleIndDir(fs, sb, in, d_block_num, &bytes_read, new_path);
        }
    }

    //triple
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

                copySingleIndDir(fs, sb, in, d_block_num, &bytes_read, new_path);
            }
        }
    }

}
