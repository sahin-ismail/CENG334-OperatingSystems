#include <vector>
#include <string.h>
#include <fcntl.h>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include "ext2fs.h"
#include <sys/mman.h>

typedef unsigned char bmap;
unsigned int block_size = 0;

#define BITMAPSET(d, set) ((set[((d) / (8 * (int) sizeof (bmap)))] |= ((bmap) 1 << ((d) % (8 * (int) sizeof (bmap))))))
#define BITMAPISSET(d, set) ((set[((d) / (8 * (int) sizeof (bmap)))] & ((bmap) 1 << ((d) % (8 * (int) sizeof (bmap))))) != 0)
#define BITMAPCLEAR(d, set) ((set[((d) / (8 * (int) sizeof (bmap)))] &= ~((bmap) 1 << ((d) % (8 * (int) sizeof (bmap))))))
#define BS_OFFSET 1024
#define BLK_OFFSET(block) (block*block_size)
#define F_OFFSET 2048

struct ext2_inode new_inode_deneme,new_inode_deneme1, new_inode,parent_new_inode, file_new_inode;
struct ext2_super_block super;
struct ext2_group_desc group;
bmap* block_bitmap; 
bmap* inode_bitmap;
int indirect_block_no = 0, indirect_offset = 0, double_offset = 0, triple_offset = 0, inode_no = 2, inode_index, containing_group_id,  block_counter = 0 , inode_indice, double_block_no = 0, triple_block_no = 0, image, currrent_gid, number_of_groups, new_inode_gid, target_inode_no,parent_inode, file_inode, test1, test2;
unsigned int indirect_block_index, double_block_index, triple_block_index, indirect_block_counter = 0, double_block_counter = 0, triple_block_counter = 0, indirect_block_c = 0, double_block_c = 0, triple_block_c = 0, block_no, allocate_count = 0, dup_size = 0;
bool first_indirect = true, first_double = true, first_triple = true, is_full = false;
unsigned char *disk;

unsigned int insertToBlock(unsigned char *);
int getInode(char *);
void switchGroup(int, bool);
void getNewSpaceForInode();
void insertToTarget(char *);
size_t entrySize(unsigned int);
size_t entrySize(struct ext2_dir_entry*);
void remove_inode_bitmap(unsigned int , unsigned char *,int );
void remove_block_bitmap(unsigned int , unsigned char *, int );
int getInodeFromInode(int ,char* );



int main(int argc, char* argv[]) {

    if(strcmp(argv[1], "dup") == 0){

        // open image
        if ((image = open(argv[2], O_RDWR)) < 0) {
            fprintf(stderr, "Error\n");
            return 1;
        }

        // read superblock
        lseek(image, BS_OFFSET, SEEK_SET);
        read(image, &super, sizeof(super));
        if (super.magic != EXT2_SUPER_MAGIC) {
            fprintf(stderr, "Error\n");
            return 1;
        }
        block_size = 1 << (10 + super.log_block_size);

        block_bitmap = new bmap[block_size];
        inode_bitmap = new bmap[block_size];

       
        number_of_groups = (super.inodes_per_group + super.inodes_count - 1)/super.inodes_per_group;
        switchGroup(0, true);

        char *fileName;
        char *target;

        //dest filename
        char temp[EXT2_MAX_NAME_LENGTH];
        strncpy(temp, argv[4], EXT2_MAX_NAME_LENGTH);
        char *ptr = strtok(temp, "/");
        while(ptr) {
            fileName = ptr;
            ptr = strtok(NULL, "/");
        }

        target = argv[4];
        
        int lengthInput = strlen(argv[4]);
        int lengthName = strlen(fileName);

        target[lengthInput-lengthName-1] = '\0';


        if(argv[3][0] == '/'){
            file_inode = getInode(argv[3]);
            switchGroup(0, true);
        }else{
            file_inode = atoi(argv[3]);
        }

        if(target[0] == '/'){
            target_inode_no = getInode(target);
        }else{
            target_inode_no = atoi(target);
        }


        //dup file
        switchGroup(0, false);
        getNewSpaceForInode();

        std::cout << inode_no << "\n";

        inode_index = (inode_no - 1) % super.inodes_per_group;
        lseek(image, BLK_OFFSET(group.inode_table) + (inode_index * sizeof(struct ext2_inode)), SEEK_SET);
        read(image, &new_inode, sizeof(struct ext2_inode));

        inode_indice = (file_inode - 1) % super.inodes_per_group;
        lseek(image, BLK_OFFSET(group.inode_table) + (inode_indice * sizeof(struct ext2_inode)), SEEK_SET);
        read(image, &file_new_inode, sizeof(struct ext2_inode));

        new_inode.mode = file_new_inode.mode;
        new_inode.uid = file_new_inode.uid;
        new_inode.size = file_new_inode.size;
        new_inode.atime = file_new_inode.atime;
        new_inode.ctime = file_new_inode.ctime;
        new_inode.mtime = file_new_inode.mtime;
        new_inode.dtime = file_new_inode.dtime;
        new_inode.gid = file_new_inode.gid;
        new_inode.links_count = file_new_inode.links_count;
        new_inode.flags = file_new_inode.flags;
        new_inode.osd1 = file_new_inode.osd1;
        new_inode.generation = file_new_inode.generation;
        new_inode.file_acl = file_new_inode.file_acl;
        new_inode.dir_acl = file_new_inode.dir_acl;
        new_inode.faddr = file_new_inode.faddr;


        for (int i = 0; i < 3; ++i)
        {
            new_inode.extra[i] = file_new_inode.extra[i];
        }


        for (int i = 0; i < 15; ++i)
        {
            new_inode.block[i] = file_new_inode.block[i];
        }


        insertToTarget(fileName);

        if (is_full){
            new_inode.size = dup_size;
        }

        //write changes to image
        lseek(image, BLK_OFFSET(group.inode_bitmap), SEEK_SET);
        write(image, inode_bitmap, block_size);

        lseek(image, BLK_OFFSET(group.block_bitmap), SEEK_SET);
        write(image, block_bitmap, block_size);

        if (block_size == 4096){
            lseek(image, BS_OFFSET + sizeof(super)+ F_OFFSET + sizeof(group)*currrent_gid, SEEK_SET);
        }
        else{
            lseek(image, BS_OFFSET + sizeof(super)+sizeof(group)*currrent_gid, SEEK_SET);
        }

        write(image, &group, sizeof(group));

        lseek(image, BS_OFFSET, SEEK_SET);
        write(image, &super, sizeof(super));

        switchGroup(new_inode_gid, false);
        inode_index = (inode_no - 1) % super.inodes_per_group;
        lseek(image, BLK_OFFSET(group.inode_table) + (inode_index * sizeof(struct ext2_inode)), SEEK_SET);
        write(image, &new_inode, sizeof(struct ext2_inode));

        //free moemory
        delete [] block_bitmap;
        delete [] inode_bitmap;


        close(image);
      
    }else if(strcmp(argv[1], "rm") == 0){
        //open file
        if ((image = open(argv[2], O_RDWR)) < 0) {
            fprintf(stderr, "Error\n");
            return 1;
        }

        //read superblock
        lseek(image, BS_OFFSET, SEEK_SET);
        read(image, &super, sizeof(super));
        if (super.magic != EXT2_SUPER_MAGIC) {
            fprintf(stderr, "Error\n");
            return 1;
        }
        block_size = 1 << (10 + super.log_block_size);

        block_bitmap = new bmap[block_size];
        inode_bitmap = new bmap[block_size];

        
        number_of_groups = (super.inodes_per_group + super.inodes_count - 1)/super.inodes_per_group;
        switchGroup(0, true);

        disk = static_cast<unsigned char*>(mmap(NULL, block_size*block_size*35, PROT_READ | PROT_WRITE, MAP_SHARED, image, 0));
        if(disk == MAP_FAILED) {
            perror("mmap");
            exit(1);
        }
        char temporary[EXT2_MAX_NAME_LENGTH];
        for (int i = 0; i < strlen(argv[3]); ++i)
        {
            temporary[i] = argv[3][i];
        }

        if(argv[3][0] == '/'){
            file_inode = getInode(argv[3]);
        }else{
            char *fileName1;
            char *target1;

            //dest filename1
            char temp1[EXT2_MAX_NAME_LENGTH];
            strncpy(temp1, temporary, EXT2_MAX_NAME_LENGTH);
            char *ptr1 = strtok(temp1, "/");
            while(ptr1) {
                fileName1 = ptr1;
                ptr1 = strtok(NULL, "/");
            }

            target1 = temporary;
            
            int lengthInput1 = strlen(temporary);
            int lengthName1 = strlen(fileName1);

            target1[lengthInput1-lengthName1-1] = '\0';
            file_inode = getInodeFromInode(atoi(target1),fileName1);
        }
        std::cout << file_inode << '\n';
        


        inode_indice = (file_inode - 1) % super.inodes_per_group;
        lseek(image, BLK_OFFSET(group.inode_table) + (inode_indice * sizeof(struct ext2_inode)), SEEK_SET);
        read(image, &new_inode, sizeof(struct ext2_inode));
        switchGroup(0, true);
        char *fileName;
        char *target;

        //dest filename
        char temp[EXT2_MAX_NAME_LENGTH];
        strncpy(temp, argv[3], EXT2_MAX_NAME_LENGTH);
        char *ptr = strtok(temp, "/");
        while(ptr) {
            fileName = ptr;
            ptr = strtok(NULL, "/");
        }

        target = argv[3];
        
        int lengthInput = strlen(argv[3]);
        int lengthName = strlen(fileName);

        target[lengthInput-lengthName-1] = '\0';
        if(target[0] == '/'){
            parent_inode = getInode(target);
        }else{
            parent_inode = atoi(target);
        }


        inode_indice = (parent_inode - 1) % super.inodes_per_group;
        lseek(image, BLK_OFFSET(group.inode_table) + (inode_indice * sizeof(struct ext2_inode)), SEEK_SET);
        read(image, &parent_new_inode, sizeof(struct ext2_inode));

        struct ext2_inode *currentFile = &new_inode;
        currentFile->links_count--;
        struct ext2_inode *parentDir = &parent_new_inode;
        unsigned int *currentBlock = parentDir->block;
        unsigned int inodeNum;
        int i;

        for (i = 0; i < 12 && currentBlock[i]; i++) {
        unsigned int total_size = block_size;
        struct ext2_dir_entry *de = (struct ext2_dir_entry *)(disk + (currentBlock[i] * block_size));
            while(total_size > 0) {

                if (strncmp(fileName, de->name, de->name_len) == 0 && (de->name_len == strlen(fileName)) ) {
                    struct ext2_dir_entry *preventry = (struct ext2_dir_entry *)(disk + (currentBlock[i] * block_size));

                    while((struct ext2_dir_entry *)((char *)preventry + preventry->rec_len) != de)
                    {
                        preventry = (struct ext2_dir_entry *)((char *)preventry + preventry->rec_len);
                    }
                    preventry->rec_len += de->rec_len;
                    inodeNum = de->inode;
                    memset(de, 0, sizeof(struct ext2_dir_entry));
                    
                    break;

                }
                    total_size -= de->rec_len;
                    de = (struct ext2_dir_entry *)((char *)de + de->rec_len);
            }
            if (total_size >0) {
                break;
            }

        }



        if (currentFile->links_count == 0) {

            //removing inode bit from inode bitmap
            remove_inode_bitmap(inodeNum, disk,block_size);
            //removing inode block bits from block bitmap
            for (i = 0; i < 12 && currentBlock[i]; i++) {
                remove_block_bitmap(currentBlock[i], disk,block_size);
                std::cout << currentBlock[i] << " ";
            }

            if (currentBlock[i]) {
                std::cout << currentBlock[i] << " ";
                int j;
                unsigned int *single_indirect = (unsigned int *) (disk +  currentBlock[i] * block_size);
                for (j = 0; single_indirect[j] ; j++) {
                    remove_block_bitmap(single_indirect[j], disk,block_size);
                }
                remove_block_bitmap(currentBlock[i], disk,block_size);
            }
            //set inode to 0
            currentFile = 0;
        }else{
            std::cout << "-1";
        }
        


    }else if(strcmp(argv[1], "rd") == 0){

                
        if ((image = open(argv[2], O_RDWR)) < 0) {
            fprintf(stderr, "Error\n");
            return 1;
        }

        lseek(image, BS_OFFSET, SEEK_SET);
        read(image, &super, sizeof(super));
        if (super.magic != EXT2_SUPER_MAGIC) {
            fprintf(stderr, "Error\n");
            return 1;
        }
        block_size = 1 << (10 + super.log_block_size);

        block_bitmap = new bmap[block_size];
        inode_bitmap = new bmap[block_size];

        number_of_groups = (super.inodes_per_group + super.inodes_count - 1)/super.inodes_per_group;
        switchGroup(0, true);

        test1 = getInode(argv[3]);
        test2 = getInode(argv[4]);

        inode_indice = (test1 - 1) % super.inodes_per_group;
        lseek(image, BLK_OFFSET(group.inode_table) + (inode_indice * sizeof(struct ext2_inode)), SEEK_SET);
        read(image, &new_inode_deneme, sizeof(struct ext2_inode));
        printf("inode %d\n", test1);
        printf("mode %d\n", new_inode_deneme.mode);
        printf("uid %d\n", new_inode_deneme.uid);
        printf("gid %d\n", new_inode_deneme.gid);
        printf("size %d\n", new_inode_deneme.size);
        printf("atime %d\n", new_inode_deneme.atime);
        printf("mtime %d\n", new_inode_deneme.mtime);
        printf("ctime %d\n", new_inode_deneme.ctime);
        printf("size %d\n", new_inode_deneme.size);
        printf("size %d\n", new_inode_deneme.block[0]);
        printf("size %d\n", new_inode_deneme.block[1]);
        printf("size %d\n", new_inode_deneme.block[13]);
        printf("size %d\n", new_inode_deneme.block[14]);


        inode_indice = (test2 - 1) % super.inodes_per_group;
        lseek(image, BLK_OFFSET(group.inode_table) + (inode_indice * sizeof(struct ext2_inode)), SEEK_SET);
        read(image, &new_inode_deneme1, sizeof(struct ext2_inode));
        printf("---------------------\n");
        printf("inode %d\n", test2);
        printf("mode %d\n", new_inode_deneme1.mode);
        printf("uid %d\n", new_inode_deneme1.uid);
        printf("gid %d\n", new_inode_deneme1.gid);
        printf("size %d\n", new_inode_deneme1.size);
        printf("atime %d\n", new_inode_deneme1.atime);
        printf("mtime %d\n", new_inode_deneme1.mtime);
        printf("ctime %d\n", new_inode_deneme1.ctime);
        printf("size %d\n", new_inode_deneme1.size);
        printf("size %d\n", new_inode_deneme1.block[0]);
        printf("size %d\n", new_inode_deneme1.block[1]);
        printf("size %d\n", new_inode_deneme1.block[13]);
        printf("size %d\n", new_inode_deneme1.block[14]);
    }


    return 0;
}

void remove_block_bitmap(unsigned int block_num, unsigned char *disk, int block_size)
{
    struct ext2_super_block *super_block = (struct ext2_super_block *)(disk + block_size);
    super_block->free_blocks_count++;

    struct ext2_group_desc *group_desc = (struct ext2_group_desc *)(disk + (2 * block_size));
    char *bitmap = (char *)(disk + (group_desc->block_bitmap * block_size));
    char byte = bitmap[(block_num - 1) / 8];
    int pos = (block_num - 1) % 8;

    bitmap[(block_num - 1) / 8] = byte & ~(1 << pos);
}

void remove_inode_bitmap(unsigned int inode_num, unsigned char *disk,int block_size)
{
    struct ext2_super_block *super_block = (struct ext2_super_block *)(disk + block_size);
    super_block->free_inodes_count++;
    struct ext2_group_desc *group_desc = (struct ext2_group_desc *)(disk + (2 * block_size));


    char *bitmap = (char *)(disk + (group_desc->inode_bitmap * block_size));

    char byte = bitmap[(inode_num - 1) / 8];

    int pos = (inode_num - 1) % 8;
    

    bitmap[(inode_num - 1) / 8] = byte & ~(1 << pos);

}

int getInodeFromInode(int id,char* name) {

        std::vector<char*> path;
        path.push_back(name);

 

        bool found = false, innerFlag;
        unsigned char block[block_size];
        int temp_inode_no = id, pathIndex = 0;
        struct ext2_inode tempInode;

        while (!found) {
            // Read temp_inode_no's inode into tempInode
            inode_index = (temp_inode_no - 1) % super.inodes_per_group;
            lseek(image, BLK_OFFSET(group.inode_table) + (inode_index * sizeof(struct ext2_inode)), SEEK_SET);
            read(image, &tempInode, sizeof(struct ext2_inode));

            // Traverse tempInode's data blocks
            innerFlag = false;
            for (int i = 0; i < 12 ; ++i) {
                unsigned int size = 0;
                lseek(image, BLK_OFFSET(tempInode.block[i]), SEEK_SET);
                read(image, block, block_size);
                struct ext2_dir_entry* entry = (struct ext2_dir_entry*)block;

                // Traverse dir entries of data block i
                while (size < block_size) { // && entry->inode
                    if (!strncmp(entry->name, path[pathIndex], strlen(path[pathIndex]))) {
                        if (pathIndex+1 == path.size()) {
                            found = true;
                        }
                        temp_inode_no = entry->inode; // switch inode
                        innerFlag = true;
                        break;
                    }
                    size += entry->rec_len;
                    entry = (ext2_dir_entry*)((char*)entry + entry->rec_len);
                }
                if (innerFlag)
                    break;
            }
            pathIndex++;
            containing_group_id = (temp_inode_no - 1) / super.inodes_per_group;
            switchGroup(containing_group_id, false);
        }
        return temp_inode_no;
}

int getInode(char * target) {
    int num = atoi(target);
    if (num)
        return num;
    else if (!strncmp(target, "/", EXT2_MAX_NAME_LENGTH))
        return 2;
    else {
        std::vector<char*> path;
        char temp[255];
        strncpy(temp, target, 255);
        char *ptr = strtok(temp, "/");
        while(ptr) {
            path.push_back(ptr);
            ptr = strtok(NULL, "/");
        }

        bool found = false, innerFlag;
        unsigned char block[block_size];
        int temp_inode_no = 2, pathIndex = 0;
        struct ext2_inode tempInode;

        while (!found) {
            // Read temp_inode_no's inode into tempInode
            inode_index = (temp_inode_no - 1) % super.inodes_per_group;
            lseek(image, BLK_OFFSET(group.inode_table) + (inode_index * sizeof(struct ext2_inode)), SEEK_SET);
            read(image, &tempInode, sizeof(struct ext2_inode));

            // Traverse tempInode's data blocks
            innerFlag = false;
            for (int i = 0; i < 12 ; ++i) {
                unsigned int size = 0;
                lseek(image, BLK_OFFSET(tempInode.block[i]), SEEK_SET);
                read(image, block, block_size);
                struct ext2_dir_entry* entry = (struct ext2_dir_entry*)block;

                // Traverse dir entries of data block i
                while (size < block_size) { // && entry->inode
                    if (!strncmp(entry->name, path[pathIndex], strlen(path[pathIndex]))) {
                        if (pathIndex+1 == path.size()) {
                            found = true;
                        }
                        temp_inode_no = entry->inode; // switch inode
                        innerFlag = true;
                        break;
                    }
                    size += entry->rec_len;
                    entry = (ext2_dir_entry*)((char*)entry + entry->rec_len);
                }
                if (innerFlag)
                    break;
            }
            pathIndex++;
            containing_group_id = (temp_inode_no - 1) / super.inodes_per_group;
            switchGroup(containing_group_id, false);
        }
        return temp_inode_no;
    }
}


void switchGroup(int bgID, bool first) {

    if (bgID >= number_of_groups) {
        fprintf(stderr, "Error\n");
        exit(3);
    }
    else if (!first && bgID == currrent_gid){
        return;
    }

    if (!first) {
        if (block_size == 4096){
            lseek(image, BS_OFFSET+sizeof(super)+ F_OFFSET + sizeof(group)*currrent_gid, SEEK_SET);
        }
        else{
            lseek(image, BS_OFFSET+sizeof(super) + sizeof(group)*currrent_gid, SEEK_SET);
        }

        write(image, &group, sizeof(group));

        lseek(image, BLK_OFFSET(group.block_bitmap), SEEK_SET);
        write(image, block_bitmap, block_size);

        lseek(image, BLK_OFFSET(group.inode_bitmap), SEEK_SET);
        write(image, inode_bitmap, block_size);
    }

    currrent_gid = bgID;

    if (block_size == 4096){
        lseek(image, BS_OFFSET+sizeof(super)+ F_OFFSET + sizeof(group)*currrent_gid, SEEK_SET);
    }
    else{
        lseek(image, BS_OFFSET+sizeof(super) + sizeof(group)*currrent_gid, SEEK_SET);
    }

    read(image, &group, sizeof(group));

    lseek(image, BLK_OFFSET(group.block_bitmap), SEEK_SET);
    read(image, block_bitmap, block_size);

    lseek(image, BLK_OFFSET(group.inode_bitmap), SEEK_SET);
    read(image, inode_bitmap, block_size);
}

void getNewSpaceForInode() {

    while (true) {
        inode_index = (inode_no - 1) % super.inodes_per_group;
        if (!BITMAPISSET(inode_index, inode_bitmap)) {
            containing_group_id = (inode_no - 1) / super.inodes_per_group;
            new_inode_gid = containing_group_id;
            BITMAPSET(inode_index, inode_bitmap);
            group.free_inodes_count--;
            super.free_inodes_count--;
            lseek(image, BLK_OFFSET(group.inode_table) + (inode_index * sizeof(struct ext2_inode)), SEEK_SET);
            read(image, &new_inode, sizeof(struct ext2_inode));
            break;
        }
        if (inode_no && !(inode_no % super.inodes_per_group)) {
            // Block group border reached, Switch group
            inode_no++;
            containing_group_id = (inode_no - 1) / super.inodes_per_group;
            switchGroup(containing_group_id, false);
        }
        else
            inode_no++;
    }
}

void insertToTarget(char * fileName) {

    containing_group_id = (target_inode_no - 1) / super.inodes_per_group;
    switchGroup(containing_group_id, false);

    // go to targetInode
    struct ext2_inode targetInode;
    unsigned int target_inode_index = (target_inode_no - 1) % super.inodes_per_group, tGid = currrent_gid;
    lseek(image, BLK_OFFSET(group.inode_table) + (target_inode_index * sizeof(struct ext2_inode)), SEEK_SET);
    read(image, &targetInode, sizeof(struct ext2_inode));

    const size_t newEntrySize = entrySize(strlen(fileName));

    // Traverse targetInode's data blocks
    unsigned char buff[block_size];
    unsigned int size;
    bool found = false;
    for (int i = 0; i < 12 ; ++i) {

        if (!targetInode.block[i]) {

            memset(buff, 0, block_size);
            struct ext2_dir_entry* newLastEntry = (struct ext2_dir_entry*)((char*)buff);
            newLastEntry->inode = inode_no;
            newLastEntry->file_type = 1;
            strncpy(newLastEntry->name, fileName, strlen(fileName));
            newLastEntry->name_len = strlen(fileName);
            newLastEntry->rec_len = block_size;

            targetInode.block[i] = insertToBlock(buff);
            std::cout << targetInode.block[i];
            switchGroup(tGid, false);
            lseek(image, BLK_OFFSET(group.inode_table) + (target_inode_index * sizeof(struct ext2_inode)), SEEK_SET);
            write(image, &targetInode, sizeof(struct ext2_inode));
            break;
        }



        lseek(image, BLK_OFFSET(targetInode.block[i]), SEEK_SET);
        read(image, buff, block_size);
        // convert data block to entry
        struct ext2_dir_entry* entry = (struct ext2_dir_entry*) buff;

        // Traverse dir entries of data block i
        size = 0;
        while (size < block_size) {
            const size_t realEntrySize = entrySize(entry);
            if (entry->rec_len > realEntrySize) {

                if (realEntrySize + newEntrySize <= entry->rec_len - realEntrySize) {

                    struct ext2_dir_entry* newLastEntry = (struct ext2_dir_entry*)((char*)entry + realEntrySize);
                    newLastEntry->inode = inode_no;
                    newLastEntry->file_type = 1;
                    strncpy(newLastEntry->name, fileName, strlen(fileName));
                    newLastEntry->name_len = strlen(fileName);
                    newLastEntry->rec_len = entry->rec_len - realEntrySize;
                    entry->rec_len = realEntrySize;

                    lseek(image, BLK_OFFSET(targetInode.block[i]), SEEK_SET);
                    write(image, buff, block_size);
                    found = true;
                    break;
                }
            }


            size += entry->rec_len;
            entry = (ext2_dir_entry*)((char*)entry + entry->rec_len);
        }

        if (found){
            std::cout << (-1) ;
            break;
        }
    }
}



unsigned int insertToBlock(unsigned char * block) {


    unsigned char buf[block_size];
    unsigned int * bPtr;
    unsigned int bitIndex, blockIndex, retVal;

    while (true) {

        if (block_no) {

            if (block_size == 1024) {
                bitIndex = (block_no - 1) % super.blocks_per_group;
                blockIndex = block_no - 1;
            }
            else {
                bitIndex = block_no % super.blocks_per_group;
                blockIndex = block_no;
            }

        }
        else {
            blockIndex = 0;
            bitIndex = 0;
        }

        if (!BITMAPISSET(bitIndex, block_bitmap)) {
            allocate_count++;
            dup_size += block_size;
            group.free_blocks_count--;
            super.free_blocks_count--;
            BITMAPSET(bitIndex, block_bitmap);

            if (block_counter == 12) {
                if (indirect_block_counter == 0 && first_indirect) {

                    unsigned char indBlock[block_size];
                    memset(indBlock, 0, block_size);
                    indirect_block_index = blockIndex;

                    if (block_size == 1024){
                        lseek(image, BLK_OFFSET(super.first_data_block) + (block_size * indirect_block_index), SEEK_SET);
                    }
                    else{
                        lseek(image, BLK_OFFSET(indirect_block_index), SEEK_SET);
                    }

                    write(image, indBlock, block_size);

                    indirect_block_no = block_no;
                    indirect_block_c = block_no;
                    block_no++;
                    first_indirect = false;

                    continue;
                }

                if (block_size == 1024){
                    lseek(image, BLK_OFFSET(super.first_data_block) + (block_size * indirect_block_index), SEEK_SET);
                }
                else{
                    lseek(image, BLK_OFFSET(indirect_block_index), SEEK_SET);
                }

                read(image, buf, block_size);

                bPtr = (unsigned int*)(buf + (indirect_offset * sizeof(unsigned int)));
                *bPtr = block_no;
                indirect_offset++;

                if (block_size == 1024){
                    lseek(image, BLK_OFFSET(super.first_data_block) + (block_size * indirect_block_index), SEEK_SET);
                }
                else{
                    lseek(image, BLK_OFFSET(indirect_block_index), SEEK_SET);
                }

                write(image, buf, block_size);

                indirect_block_counter++;

                if (indirect_block_counter == block_size/4) {
                    indirect_block_counter = 0;
                    indirect_offset = 0;
                    first_indirect = true;
                    block_counter++;
                }

                retVal = indirect_block_no;
            }

            else if (block_counter == 13) {
                if (double_block_counter == 0 && first_double) {

                    unsigned char indBlock[block_size];
                    memset(indBlock, 0, block_size);
                    double_block_index = blockIndex;

                    if (block_size == 1024){
                        lseek(image, BLK_OFFSET(super.first_data_block) + (block_size * double_block_index), SEEK_SET);
                    }
                    else{
                        lseek(image, BLK_OFFSET(double_block_index), SEEK_SET);
                    }

                    write(image, indBlock, block_size);

                    double_block_no = block_no;
                    double_block_c = block_no;
                    block_no++;
                    first_double = false;
                    continue;
                }

                if (indirect_block_counter == 0 && first_indirect) {
                    unsigned char indBlock[block_size];
                    memset(indBlock, 0, block_size);
                    indirect_block_index = blockIndex;

                    if (block_size == 1024){
                        lseek(image, BLK_OFFSET(super.first_data_block) + (block_size * indirect_block_index), SEEK_SET);
                    }
                    else{
                        lseek(image, BLK_OFFSET(indirect_block_index), SEEK_SET);
                    }

                    write(image, indBlock, block_size);

                    indirect_block_no = block_no;
                    first_indirect = false;
                    block_no++;


                    if (block_size == 1024){
                        lseek(image, BLK_OFFSET(super.first_data_block) + (block_size * double_block_index), SEEK_SET);
                    }
                    else{
                        lseek(image, BLK_OFFSET(double_block_index), SEEK_SET);
                    }

                    read(image, buf, block_size);

                    bPtr = (unsigned int*)(buf + (double_offset * sizeof(unsigned int)));
                    *bPtr = indirect_block_no;
                    double_offset++;

                    if (block_size == 1024){
                        lseek(image, BLK_OFFSET(super.first_data_block) + (block_size * double_block_index), SEEK_SET);
                    }
                    else{
                        lseek(image, BLK_OFFSET(double_block_index), SEEK_SET);
                    }

                    write(image, buf, block_size);

                    double_block_counter++;
                    if (double_block_counter == block_size/4) {
                        indirect_offset = 0;
                        indirect_block_counter = 0;
                        double_offset = 0;
                        double_block_counter = 0;
                        block_counter++;
                        first_indirect = true;
                        first_double = true;
                    }
                    continue;
                }


                if (block_size == 1024){
                    lseek(image, BLK_OFFSET(super.first_data_block) + (block_size * indirect_block_index), SEEK_SET);
                }
                else{
                    lseek(image, BLK_OFFSET(indirect_block_index), SEEK_SET);
                }

                read(image, buf, block_size);

                bPtr = (unsigned int*)(buf + (indirect_offset * sizeof(unsigned int)));
                *bPtr = block_no;
                indirect_offset++;

                if (block_size == 1024){
                    lseek(image, BLK_OFFSET(super.first_data_block) + (block_size * indirect_block_index), SEEK_SET);
                }
                else{
                    lseek(image, BLK_OFFSET(indirect_block_index), SEEK_SET);
                }

                write(image, buf, block_size);

                indirect_block_counter++;

                if (indirect_block_counter == block_size/4) {
                    indirect_offset = 0;
                    indirect_block_counter = 0;
                    first_indirect = true;
                }
                retVal = double_block_no;
            }

            else if (block_counter == 14) {
                if (triple_block_counter == 0 && first_triple) {
                    unsigned char indBlock[block_size];
                    memset(indBlock, 0, block_size);
                    triple_block_index = blockIndex;

                    if (block_size == 1024){
                        lseek(image, BLK_OFFSET(super.first_data_block) + (block_size * triple_block_index), SEEK_SET);
                    }
                    else{
                        lseek(image, BLK_OFFSET(triple_block_index), SEEK_SET);
                    }

                    write(image, indBlock, block_size);

                    triple_block_no = block_no;
                    triple_block_c = block_no;
                    block_no++;
                    first_triple = false;
                    is_full = true;
                    continue;
                }

                if (double_block_counter == 0 && first_double) {
                    unsigned char indBlock[block_size];
                    memset(indBlock, 0, block_size);
                    double_block_index = blockIndex;

                    if (block_size == 1024){
                        lseek(image, BLK_OFFSET(super.first_data_block) + (block_size * double_block_index), SEEK_SET);
                    }
                    else{
                        lseek(image, BLK_OFFSET(double_block_index), SEEK_SET);
                    }

                    write(image, indBlock, block_size);

                    double_block_no = block_no;
                    first_double = false;
                    block_no++;


                    if (block_size == 1024){
                        lseek(image, BLK_OFFSET(super.first_data_block) + (block_size * triple_block_index), SEEK_SET);
                    }
                    else{
                        lseek(image, BLK_OFFSET(triple_block_index), SEEK_SET);
                    }

                    read(image, buf, block_size);

                    bPtr = (unsigned int*)(buf + (triple_offset * sizeof(unsigned int)));
                    *bPtr = double_block_no;
                    triple_offset++;

                    if (block_size == 1024){
                        lseek(image, BLK_OFFSET(super.first_data_block) + (block_size * triple_block_index), SEEK_SET);
                    }
                    else{
                        lseek(image, BLK_OFFSET(triple_block_index), SEEK_SET);
                    }

                    write(image, buf, block_size);

                    triple_block_counter++;
                    if (triple_block_counter == block_size/4) {
                        fprintf(stderr, "Error\n");
                        exit(3);
                    }
                    continue;
                }

                if (indirect_block_counter == 0 && first_indirect) {
                    unsigned char indBlock[block_size];
                    memset(indBlock, 0, block_size);
                    indirect_block_index = blockIndex;

                    if (block_size == 1024){
                        lseek(image, BLK_OFFSET(super.first_data_block) + (block_size * indirect_block_index), SEEK_SET);
                    }
                    else{
                        lseek(image, BLK_OFFSET(indirect_block_index), SEEK_SET);
                    }

                    write(image, indBlock, block_size);

                    indirect_block_no = block_no;
                    first_indirect = false;
                    block_no++;


                    if (block_size == 1024){
                        lseek(image, BLK_OFFSET(super.first_data_block) + (block_size * double_block_index), SEEK_SET);
                    }
                    else{
                        lseek(image, BLK_OFFSET(double_block_index), SEEK_SET);
                    }

                    read(image, buf, block_size);

                    bPtr = (unsigned int*)(buf + (double_offset * sizeof(unsigned int)));
                    *bPtr = indirect_block_no;
                    double_offset++;

                    if (block_size == 1024){
                        lseek(image, BLK_OFFSET(super.first_data_block) + (block_size * double_block_index), SEEK_SET);
                    }
                    else{
                        lseek(image, BLK_OFFSET(double_block_index), SEEK_SET);
                    }

                    write(image, buf, block_size);

                    double_block_counter++;
                    if (double_block_counter == block_size/4) {
                        double_offset = 0;
                        double_block_counter = 0;
                        first_double = true;
                    }
                    continue;
                }


                if (block_size == 1024){
                    lseek(image, BLK_OFFSET(super.first_data_block) + (block_size * indirect_block_index), SEEK_SET);
                }
                else{
                    lseek(image, BLK_OFFSET(indirect_block_index), SEEK_SET);
                }

                read(image, buf, block_size);

                bPtr = (unsigned int*)(buf + (indirect_offset * sizeof(unsigned int)));
                *bPtr = block_no;
                indirect_offset++;

                if (block_size == 1024){
                    lseek(image, BLK_OFFSET(super.first_data_block) + (block_size * indirect_block_index), SEEK_SET);
                }
                else{
                    lseek(image, BLK_OFFSET(indirect_block_index), SEEK_SET);
                }

                write(image, buf, block_size);

                indirect_block_counter++;
                if (indirect_block_counter == block_size/4) {
                    indirect_offset = 0;
                    indirect_block_counter = 0;
                    first_indirect = true;
                }
                retVal = triple_block_no;
            }

            else {

                block_counter++;
                retVal = block_no;
            }

            if (block_size == 1024){
                lseek(image, BLK_OFFSET(super.first_data_block) + (block_size * blockIndex), SEEK_SET);
            }
            else{
                lseek(image, BLK_OFFSET(blockIndex), SEEK_SET);
            }

            write(image, block, block_size);
            break;
        }

        if (block_no && !(block_no % super.blocks_per_group)) {
            // Block group border reached, Switch group
            block_no++;
            containing_group_id = (block_no - 1) / super.blocks_per_group;
            switchGroup(containing_group_id, false);
        }
        else
            block_no++;
    }
    return retVal;
}

size_t entrySize(unsigned int name_len) {

    size_t result = (8 + name_len + 3) & (~3);

    return result;
}

size_t entrySize(struct ext2_dir_entry* entry) {

    size_t result = (8 + entry->name_len + 3) & (~3);

    return result;
}