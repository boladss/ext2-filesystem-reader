// TO DO: refactor
//        add error handlers

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned int uint;
typedef unsigned char uchar;


#define SB_START 1024

typedef struct superblock{
    uint block_sz; // byte offsets 24-27
    uint num_of_blocks; // byte offsets 32-35
    uint num_of_inodes; // byte offsets 40-43
    uint inode_sz; // byte offsets 88-89
    uint bgdt_block_num; // block after superblock
} superblock;

typedef struct inode{
    uint addr; // starting address
    uint isDir; // determined by bytes 0-1, value of 0x4000 when it is a directory 
    uint file_sz; // byte offsets 4-7
    uint direct[12]; // byte offsets 40-87, 12 direct block numbers
    uint single_ind; // byte offsets 88-91, singly indirect block number
    uint double_ind; // byte offsets 92-95, doubly indirect block number
    uint triple_ind; // byte offsets 96-99, triply indirect block number
} inode;

void parseDirInode(FILE *, superblock *, inode *, char *);

// reads file and returns an unsigned int based on data read
uint readInt(FILE *fs, uint offset, uint size){
    uchar buffer[size];
    uint ret = 0;

    fseek(fs, offset, SEEK_SET);
    fread(buffer, size, 1, fs);

    //little endian
    for(int i = 0; i < size; i++){
        ret |= buffer[i] << (i*8);
    }

    return ret;
}

superblock * parseSuperBlock(FILE *fs){
    superblock * sb = (superblock *) malloc(sizeof(superblock));

    sb->block_sz = SB_START << readInt(fs, SB_START+24, 4);
    sb->num_of_blocks = readInt(fs, SB_START+32, 4);
    sb->num_of_inodes = readInt(fs, SB_START+40, 4);
    sb->bgdt_block_num = (SB_START / sb->block_sz) + 1;
    sb->inode_sz = readInt(fs, SB_START+88, 2);

    return sb;
}

// get inode data using inode number
inode * getInode(FILE * fs, superblock * sb, uint i_num){
    uchar buffer[sb->block_sz];

    uint block_group_num = (i_num - 1) / sb->num_of_inodes; // which block group the inode is in
    uint entry_addr = (sb->bgdt_block_num*sb->block_sz) + (block_group_num*32) + 8;
    uint inode_table_start = readInt(fs, entry_addr, 4); // starting block of inode table
    uint inode_index = (i_num - 1) % sb->num_of_inodes; // index of inode in table
    uint inode_addr = (inode_table_start * sb->block_sz) + (inode_index*sb->inode_sz); // addr of inode entry

    //parse inode data
    inode * in = (inode *) malloc(sizeof(inode));

    in->addr = inode_addr;
    in->isDir = (0x4000 & readInt(fs, inode_addr, 2)) >> 14;
    in->file_sz = readInt(fs, inode_addr+4, 4);
    
    //number of blocks occupied by file; ceiling function
    uint num_blocks = ((in->file_sz + sb->block_sz - 1) / sb->block_sz); 
    
    // direct pointers
    for(int i = 0; i < 12; i++){
        in->direct[i] = readInt(fs, inode_addr+40+(i*4), 4);
    }

    // indirect pointers
    in->single_ind = readInt(fs, inode_addr+88, 4);
    in->double_ind = readInt(fs, inode_addr+92, 4);
    in->triple_ind = readInt(fs, inode_addr+96, 4);

    return in;
}

// get data block using block number
void getDataBlock(FILE * fs, superblock * sb, uint block, uchar * buffer){
    uint addr = sb->block_sz * block;

    fseek(fs, addr, SEEK_SET);
    fread(buffer, sb->block_sz, 1, fs);

    return;
}

// print directory entry path based on address
void printDirContents(FILE * fs, superblock * sb, uint addr, char * path){
    uint i_num = readInt(fs, addr, 4); // get inode number

    if(i_num == 0) return; // skip empty/invalid inodes

    uint dir_len = readInt(fs, addr+6, 1); // get dir name length
    char file_name[dir_len+1];
            
    //get directory name
    fseek(fs, addr+8, SEEK_SET);
    fread(file_name, dir_len, 1, fs);
    file_name[dir_len] = '\0';

    if(strcmp(file_name, ".") && strcmp(file_name, "..")){  // exclude own and parent directory
        inode * in = getInode(fs, sb, i_num);

        // for storing new path
        char buffer[4096];
        buffer[0] = '\0'; // interpret as string

        // append dir_name to path
        strcat(buffer, path);
        strcat(buffer, file_name);

        // go into dir
        if(in->isDir){
            parseDirInode(fs, sb, in, buffer);
        }
        else{
            // print regular file path
            printf("%s\n", buffer);
        }

        free(in);
    }
}

// parses each directory entry from directory data block
void parseDirEntries(FILE * fs, superblock * sb, inode * in, uint addr, uint * bytes_read, char * path){
    uint curr_addr;
    uint offset = 0;

    // read directory data block                      
    // stops reading when number of bytes read is larger than file
    // or when offset is bigger than block size
    while(*bytes_read < in->file_sz && (offset < sb->block_sz)){               
        curr_addr = addr + offset;

        printDirContents(fs, sb, curr_addr, path);
            
        offset += readInt(fs, curr_addr+4, 2); //directory entry size
        *bytes_read += readInt(fs, curr_addr+4, 2);
    }
}


void parseSingleIndirect(FILE * fs, superblock * sb, inode *in, uint * bytes_read, char * path){
    // iterate through all direct pointers in block
    // pointer size is 4 bytes
    for(int i = 0; i < sb->block_sz; i+=4){
        uint curr_offset = (in->single_ind*sb->block_sz)+i;
        uint curr_addr = readInt(fs, curr_offset, 4) * 4096;

        //skip empty pointers
        if(curr_addr == 0) continue;

        parseDirEntries(fs, sb, in, curr_addr, bytes_read, path);
    }
}


// parse directory block entries using inode
void parseDirInode(FILE * fs, superblock * sb, inode * in, char * path){
    uint bytes_read = 0; // total number of bytes read 

    char new_path[4096];
    new_path[0] = '/';
    new_path[1] = '\0';
    strcat(path, new_path);

    // print directory path
    printf("%s\n", path);

    //direct pointers to data block
    for(int i = 0; i < 12; i++){
        if(in->direct[i] == 0) continue; //skip empty pointers

        parseDirEntries(fs, sb, in, in->direct[i]*sb->block_sz, &bytes_read, path);
    }

    //single
    if(in->single_ind){
        // handler for single indirect block
        parseSingleIndirect(fs, sb, in, &bytes_read, path);
    }
    
    //double
    if(in->double_ind){ 
        // handler for double indirect block
        // contains pointers to block containing singly indirect blocks

        for(int i = 0; i < sb->block_sz; i+= 4){ // pointer size is 4 bytes
            parseSingleIndirect(fs, sb, in, &bytes_read, path);
        }
    }

    //triple
    if(in->triple_ind){
        // handler for triple indirect block
        // contains pointers to blocks containing doubly indirect blocks

        for(int i = 0; i < sb->block_sz; i+= 4){ // pointer size is 4 bytes
            for(int i = 0; i < sb->block_sz; i+= 4){ // pointer size is 4 bytes
                parseSingleIndirect(fs, sb, in, &bytes_read, path);
            }
        }
    }

}


// references used: 
// https://wiki.osdev.org/Ext2
//    - inode computation
//    - inode flag for dir