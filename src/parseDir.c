#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char uchar;
typedef unsigned int uint;

#define SB_START 1024 //offset from start of volume to start of superblock.
//coincidentally also the size of the superblock.

typedef struct superblock{
    uint block_sz; // byte offsets 24-27; byte size of a block
    uint num_of_blocks; // byte offsets 32-35; number of blocks in a block group
    uint num_of_inodes; // byte offsets 40-43; 
    uint inode_sz; // byte offsets 88-89;
    uint bgdt_block_num; // block after superblock;
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

typedef struct dir_entry {
    uint inum; // byte offets 0-3, inode number
    uint size; // bytes 4-5, size of dir entry in bytes
    uint name_sz; // byte 6, size of name in bytes
    char * name; // name of directory
} dir_entry;

void parseDirInode(int, superblock *, inode *, char *);

// reads file and returns an uint based on data read
int readInt(int fs, uint offset, uint size){
    uchar buffer[size];
    uint ret = 0;

    lseek(fs, offset, SEEK_SET);
    read(fs, buffer, size);

    //little endian
    for(int i = 0; i < size; i++){
        ret |= buffer[i] << (i*8);
    }

    return ret;
}

superblock * parseSuperBlock(int fs){
    superblock * sb = (superblock *) malloc(sizeof(superblock));

    sb->block_sz = SB_START << readInt(fs, SB_START+24, 4);
    sb->num_of_blocks = readInt(fs, SB_START+32, 4);
    sb->num_of_inodes = readInt(fs, SB_START+40, 4);
    sb->bgdt_block_num = (sb->block_sz == 1024) ? 2 : 1;
    //block size is a multiple of 1024, minimum 1024.
    //bgdt starts at block number 2 if block size is 1024, and 1 otherwise.
    sb->inode_sz = readInt(fs, SB_START+88, 2);

    return sb;
}

// get inode data using inode number
inode * getInode(int fs, superblock * sb, uint i_num){
    const uint block_group_num = (i_num - 1) / sb->num_of_inodes; // which block group the inode is in
    const uint bgd_addr = (sb->bgdt_block_num*sb->block_sz) + (block_group_num*32) + 8; // copy of bgdt will be same for all block groups
    const uint inode_table_start = readInt(fs, bgd_addr, 4); // starting block of inode table
    const uint inode_index = (i_num - 1) % sb->num_of_inodes; // index of inode in table
    const uint inode_addr = (inode_table_start * sb->block_sz) + (inode_index*sb->inode_sz); // addr of inode entry

    //parse inode data
    inode * in = (inode *) malloc(sizeof(inode));

    in->addr = inode_addr;
    in->isDir = (0x4000 & readInt(fs, inode_addr, 2)) >> 14;
    in->file_sz = readInt(fs, inode_addr+4, 4);
    
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
void getDataBlock(int fs, superblock * sb, uint block, uchar * buffer){
    uint addr = sb->block_sz * block;

    lseek(fs, addr, SEEK_SET);
    read(fs, buffer, sb->block_sz);

    return;
}

// gets directory entry using address
dir_entry * getDirEntry(int fs, superblock * sb, uint addr){
    dir_entry * dir = (dir_entry *) malloc(sizeof(dir_entry));

    dir->inum = readInt(fs, addr, 4); // get inode number
    dir->size = readInt(fs, addr+4, 2); // get size
    dir->name_sz = readInt(fs, addr+6, 1);// get name size

    //get dir name
    dir->name = (char *) malloc((sizeof(char)*dir->name_sz)+1);
    lseek(fs, addr+8, SEEK_SET);
    read(fs, dir->name, dir->name_sz);
    dir->name[dir->name_sz] = '\0'; // set as string

    return dir;
}

void freeDirEntry(dir_entry * dir){
    free(dir->name);
    free(dir);
}

// print directory entry path based on address
void printDirContents(int fs, superblock * sb, uint addr, char * path){

    // get dir entry info
    dir_entry * dir = getDirEntry(fs, sb, addr);

    if(dir->inum == 0){
        freeDirEntry(dir);
        return;
    }

    if(strcmp(dir->name, ".") && strcmp(dir->name, "..")){  // exclude own and parent directory
        inode * in = getInode(fs, sb, dir->inum);

        // for storing new path
        char buffer[4096];
        buffer[0] = '\0'; // interpret as string

        // append dir_name to path
        strcat(buffer, path);
        strcat(buffer, dir->name);

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

    freeDirEntry(dir);
    return;
}

// parses each directory entry from directory data block
void parseDirEntries(int fs, superblock * sb, inode * in, uint addr, uint * bytes_read, char * path){
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


void parseSingleIndirect(int fs, superblock * sb, inode *in, uint block_num, uint * bytes_read, char * path){
    // iterate through all direct pointers in block
    // pointer size is 4 bytes
    for(int i = 0; i < sb->block_sz; i+=4){
        uint curr_offset = (block_num*sb->block_sz)+i;
        uint curr_addr = readInt(fs, curr_offset, 4) * 4096;

        //skip empty pointers
        if(curr_addr == 0) continue;

        parseDirEntries(fs, sb, in, curr_addr, bytes_read, path);
    }
}


// parse directory block entries using inode
void parseDirInode(int fs, superblock * sb, inode * in, char * path){
    uint bytes_read = 0; // total number of bytes read 

    char new_path[4096];
    new_path[0] = '/';
    new_path[1] = '\0';
    strcat(path, new_path);

    // print directory path
    printf("%s\n", path);

    // direct pointers to data block
    for(int i = 0; i < 12; i++){
        if(in->direct[i] == 0) continue; //skip empty pointers

        parseDirEntries(fs, sb, in, in->direct[i]*sb->block_sz, &bytes_read, path);
    }

    // single indirect pointer
    if(in->single_ind){
        // handler for single indirect block
        parseSingleIndirect(fs, sb, in, in->single_ind, &bytes_read, path);
    }

    // double indirect pointer
    if(in->double_ind){ 
        // handler for double indirect block
        // contains pointers to block containing singly indirect blocks

        for(int i = 0; i < sb->block_sz; i+= 4){ // pointer size is 4 bytes
            // get double block

            uint d_block = (in->double_ind*sb->block_sz)+i; // address of direct pointer
            uint d_block_num = readInt(fs, d_block, 4); // data block number

           parseSingleIndirect(fs, sb, in, d_block_num, &bytes_read, path);
        }
    }

    // triple indirect
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

                parseSingleIndirect(fs, sb, in, d_block_num, &bytes_read, path);
            }
        }
    }

}


// references used: 
// https://wiki.osdev.org/Ext2
//    - inode computation
//    - inode flag for dir