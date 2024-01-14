// TO DO: refactor
//        add error handlers

#include <stdio.h>
#include <stdlib.h>

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
    uint file_sz; // byte offsets 4-7
    uint direct[12]; // byte offsets 40-87, 12 direct block numbers
    uint single_ind; // byte offsets 88-91, singly indirect block number
    uint double_ind; // byte offsets 92-95, doubly indirect block number
    uint triple_ind; // byte offsets 96-99, triply indirect block number
} inode;

typedef struct dir_entry{
    uint inode_num; // byte offsets 0-3, inode number
    char * dir_name; // byte offset 8 onwards, name of directory
} dir_entry;

// add variables to store default values ?

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

    //get block size
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
    uint inode_num_of_blocksber = ((inode_index * sb->inode_sz) / sb->block_sz) + inode_table_start; // block containing inode
    uint inode_addr = (inode_num_of_blocksber * sb->block_sz) + (inode_index*sb->inode_sz); // addr of inode entry

    //fseek(fs, inode_addr, SEEK_SET);
    //fread(buffer, sb->inode_sz, 1, fs);

    //parse inode data
    inode * in = (inode *) malloc(sizeof(inode));

    in->addr = inode_addr;
    in->file_sz = readInt(fs, inode_addr+4, 4);
    
    uint num_blocks = in->file_sz / sb->block_sz; //number of blocks occupied by file
    // determines how many pointers are used
    
    // direct pointers
    for(int i = 0; i < num_blocks; i++){
        in->direct[i] = readInt(fs, inode_addr+40+(i*4), 4);
    }

    // if blocks > 12, use single indirect block
    if(num_blocks > 12){
        in->single_ind = readInt(fs, inode_addr+88, 4);
    }

    //if blocks > (12 + block_sz), use double indirect block
    if(num_blocks > 12 + sb->block_sz){
        in->double_ind = readInt(fs, inode_addr+92, 4);
    }

    //if blocks > (12 + block_sz + block_sz^2), use triple indirect block
    if(num_blocks > 12 + sb->block_sz + sb->block_sz*sb->block_sz){
        in->triple_ind = readInt(fs, inode_addr+96, 4);
    }

    return in;
}

// get data block using block number
void getDataBlock(FILE * fs, superblock * sb, uint block, uchar * buffer){
    uint addr = sb->block_sz * block;

    fseek(fs, addr, SEEK_SET);
    fread(buffer, sb->block_sz, 1, fs);

    return;
}


// get directory block
// parse directory block entries from directory block
void parseDirEntries(FILE * fs, superblock * sb, inode * in){
    // get directory block from inode
    // for each block print new dir_entry to array
    // FOR LATER: append to array containing all directory entries

    uint i_num;
    char dir_name[256];
    uint dir_len;
    uint curr_size;
    uint curr_addr;
    uint dir_size;

    //direct pointers
    for(int i = 0; i < 12; i++){
        if(in->direct[i] == 0) break; //skip empty pointers

        curr_size = 0;

        do{
            //read data block
            curr_addr = in->direct[i]*sb->block_sz + curr_size;

            i_num = readInt(fs, curr_addr, 4); // get inode number
            dir_len = readInt(fs, curr_addr+6, 1); // get dir name length
            dir_size = readInt(fs, curr_addr+4, 2);
            
            //get directory name
            fseek(fs, curr_addr+8, SEEK_SET);
            fread(dir_name, dir_len, 1, fs);
            dir_name[dir_len] = '\0';

            //print values to check
            printf("inode number: %d\n", i_num);
            printf("dir name: %s\n\n", dir_name);
            
            curr_size += dir_size;
        }while(curr_size < in->file_sz && (curr_size % 4096 != 0));

    }

    //single
    
    //double

    //triple
}



// references used: 
// https://wiki.osdev.org/Ext2
// https://www.nongnu.org/ext2-doc/