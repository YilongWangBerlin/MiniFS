
#include "../lib/operations.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int find_free_block(file_system *fs) {
    for (int i = 0; i < fs->s_block->num_blocks; i++) {
        if (fs->free_list[i] == 1) { 
            return i;
        }
    }
    return -1; 
}


int find_inode(file_system *fs, char *path) {
    //split the path into parts
    char* path_copy = strdup(path);
    char* token = strtok(path_copy, "/");

    int current_inode = fs->root_node;
    while (token != NULL){
        int found = 0;
        
        for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++){
            int child_inode_index = fs->inodes[current_inode].direct_blocks[i];
            //if the child inode matche the current part of the path
            // move to the child
            if (child_inode_index != -1 && strcmp(fs->inodes[child_inode_index].name, token) == 0) {
                current_inode = child_inode_index;
                found = 1;
                break;
            }
        }



        /*
        for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
            if (current->direct_blocks[i] != -1) {
                inode* next = &fs->inodes[current->direct_blocks[i]];

                if (strcmp(next->name, token) == 0) {
                    current_inode = current->direct_blocks[i];
                    found = 1;
                    break;
                }
            }
        }
        */

        if (!found){
            free(path_copy);
            return -1;
        }
        token = strtok(NULL, "/");
        // continue using the previous string as input
    }

    free(path_copy);
    return current_inode;
}


int
fs_mkdir(file_system *fs, char *path)
{
	//check if path is root
    if (strcmp(path, "/") == 0) {
        return -1;
    }
    
    char* path_copy = strdup(path);
    char* dir_name = strrchr(path_copy, '/');
    if (dir_name == NULL){
        free(path_copy);
        return -1;
    }

    *dir_name = '\0';
    dir_name++;
    //to point to the start of the new directory name
    
    //find the parent directory
    int parent_inode_index = find_inode(fs, path_copy);
    if (parent_inode_index < 0) {
        free(path_copy);
        return -1;
    }
    
    // check if directory already exists in parent
    for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
        int child_inode_index = fs->inodes[parent_inode_index].direct_blocks[i];
        if (child_inode_index != -1 && fs->inodes[child_inode_index].n_type == directory &&
            strcmp(fs->inodes[child_inode_index].name, dir_name) == 0) {
            free(path_copy);
            return -1;
        }
    }
    
    int dir_inode_index = find_free_inode(fs);
    if (dir_inode_index < 0) {
        free(path_copy);
        return -1;
    }

    //initialize the new directory inode
    fs->inodes[dir_inode_index].n_type = directory;
    strncpy(fs->inodes[dir_inode_index].name, dir_name, NAME_MAX_LENGTH);
    fs->inodes[dir_inode_index].parent = parent_inode_index;

    // link the new directory in the parent's direct_blocks
    
    for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
        if (fs->inodes[parent_inode_index].direct_blocks[i] == -1) {
            fs->inodes[parent_inode_index].direct_blocks[i] = dir_inode_index;
            break;
        }
    }
    
    
    free(path_copy);
    return 0;
}

int
fs_mkfile(file_system *fs, char *path_and_name)
{
	char *path_copy  = strdup(path_and_name);
    char *filename = strrchr(path_copy , '/');
    if (filename == NULL) {
        free(path_copy );
        return -1; 
    }
    *filename = '\0';
    filename++;

    int parent_inode_index = find_inode(fs, path_copy );
    if (parent_inode_index < 0) {
        free(path_copy );
        return -1; 
    }
    inode *parent_inode = &(fs->inodes[parent_inode_index]);

    // to chek if the file already exist
    for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
        int child_inode_index = parent_inode->direct_blocks[i];
        if (child_inode_index != -1 && strcmp(fs->inodes[child_inode_index].name, filename) == 0) {
            // && fs->inodes[child_inode_index].n_type != directory
            //&& parent_inode != fs->root_node
            //free(path_copy );
            //return -2; 
            if(fs->inodes[child_inode_index].n_type == reg_file) {
                free(path_copy);
                return -2;  
            } else if (fs->inodes[child_inode_index].n_type == directory) {
                continue;  
            }
        }
    }

    int free_inode_index = find_free_inode(fs);
    if (free_inode_index < 0) {
        free(path_copy );
        return -1; 
    }

    // initialize the new file's inode
    inode *new_file_inode = &(fs->inodes[free_inode_index]);
    inode_init(new_file_inode);
    new_file_inode->n_type = reg_file;
    strncpy(new_file_inode->name, filename, NAME_MAX_LENGTH);
    new_file_inode->parent = parent_inode_index;

    // add the new file to the parent directory
    for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
        if (parent_inode->direct_blocks[i] == -1) {
            parent_inode->direct_blocks[i] = free_inode_index;
            break;
        }
    }

    free(path_copy);
    return 0;
}

char *
fs_list(file_system *fs, char *path)
{
	int inode_index = find_inode(fs, path);
    if (inode_index < 0) {
        return NULL; 
    }
    
    inode *dir_inode = &(fs->inodes[inode_index]);
    if (dir_inode->n_type != directory) {
        return NULL; 
    }
    
    //initialize an empty string to store the result
    char *result = malloc(1);
    result[0] = '\0';
    
    // sort the direct blocks by inode number
    // 该代码基于直接数据块本身的值对目录的直接数据块的索引进行分类
    int block_indices[DIRECT_BLOCKS_COUNT];
    for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
        block_indices[i] = i;
    }
    for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
        for (int j = i + 1; j < DIRECT_BLOCKS_COUNT; j++) {
            if (dir_inode->direct_blocks[block_indices[i]] > dir_inode->direct_blocks[block_indices[j]]) {
                int temp = block_indices[i];
                block_indices[i] = block_indices[j];
                block_indices[j] = temp;
            }
        }
    }
    
    //add the names of the files and directories to the result string
    for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
        // 这样block_index就是按direct_blocks大小顺序的了
        int block_index = dir_inode->direct_blocks[block_indices[i]];
        if (block_index != -1) {
            
            inode *item_inode = &(fs->inodes[block_index]);
            char *type;
            if (item_inode->n_type == directory){
                type = "DIR";
            }else{
                type = "FIL";
            }
            char *name = item_inode->name;
            char line[NAME_MAX_LENGTH + 5]; 
            // 将格式化后的字符串写入到line中
            // 字符串中的%s将分别被type和name的值替换
            sprintf(line, "%s %s\n", type, name);
            
            // 计算出新的内存大小，并将指向新内存的指针赋值给 result
            result = realloc(result, strlen(result) + strlen(line) + 1);
            
            // 将line字符串的内容追加到result字符串的末尾
            //将两个字符串连接在一起
            strcat(result, line);
        }
    }
    
    return result;
}

int
fs_writef(file_system *fs, char *filename, char *text)
{
    int inode_index = find_inode(fs, filename);
    if (inode_index < 0) {
        return -1; //file not found
    }

    inode *file_inode = &(fs->inodes[inode_index]);
    if (file_inode->n_type != reg_file) {
        return -1; //not file
    }

    int text_length = strlen(text);
    int remaining_length = text_length;
    int written_length = 0;

    // write the text to the file's data blocks
    for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
        // 通过一个循环遍历文件的直接数据块
        // 如果某个数据块还没有被分配 即其对应的索引为-1
        // 则找到一个空闲块 并将其索引赋值给该数据块 同时将其标记为已使用 
        // 并更新文件的大小
        // 如果某个数据块已经被分配了 会继续向该数据块中写入新的文本数据
        if (file_inode->direct_blocks[i] == -1) {
            // falls die Datei bisher leer ist
            int block_index = find_free_block(fs);
            if (block_index == -1){
                return -2; // filesystem is full
            }

            file_inode->direct_blocks[i] = block_index;
            // mark the block as used
            fs->free_list[block_index] = 0; 
            fs->s_block[i].free_blocks -= 1;

            file_inode->size += MIN(remaining_length, BLOCK_SIZE);
        }

        //write to the data block

        data_block *block = &(fs->data_blocks[file_inode->direct_blocks[i]]);
        
        // 计算要复制的数据长度 选择数据块剩余可用空间中较小的一个值作为复制长度
        int copy_length = MIN(remaining_length, BLOCK_SIZE - block->size);
        
        // 将文本数据从text数组的指定位置复制到数据块
        //block->block + block->size表示数据块中当前可用位置的指针 指向未被利用的位置 前面的部分已经写入了其他的数据
        // text + written_length表示源数据中待复制的位置
        memcpy(block->block + block->size, text + written_length, copy_length);

        block->size += copy_length;

        written_length += copy_length;
        remaining_length -= copy_length;

        if (remaining_length <= 0){
            break;
        }
    }

    if (remaining_length > 0) {
        return -2; 
    }

    return written_length;
}

uint8_t *
fs_readf(file_system *fs, char *filename, int *file_size)
{

    int inode_num = find_inode(fs, filename);
    if (inode_num ==-1){
        return NULL;
    }
    inode *file_inode = &(fs->inodes[inode_num]);
    
    *file_size = file_inode->size;  
    if (*file_size == 0) {
        return NULL;
    }


    uint8_t *buffer = malloc(*file_size);
    if (buffer == NULL){
        return NULL;
    }

    //read data from the direct blocks into the buffer
    int block_idx = 0;
    int read_length = 0;// to determine the position of the next data block

    // 函数会从文件的direct_blocks中读取数据 并将数据逐块复制到缓冲区中
    //它会遍历文件的direct_blocks 直到遇到文件的末尾或达到指定的文件大小 
    // 每次复制数据块时计算要复制的数据大小 并使用memcpy将数据从数据块复制到缓冲区
    // 复制完成后 会更新read_length 用于确定下一个数据块的位置
    while (file_inode->direct_blocks[block_idx] != -1 && read_length < *file_size) {
        data_block *block = &(fs->data_blocks[file_inode->direct_blocks[block_idx]]);
        int size_to_copy = MIN(block->size, *file_size - read_length);
        
        memcpy(buffer + read_length, block->block, size_to_copy);
        read_length += size_to_copy;
        block_idx++;


    }



    return buffer;
}



int fs_rm(file_system* fs, char* path) {
    int inode_index = find_inode(fs, path);
    if (inode_index == -1) {
        return -1; 
    }
    inode* target = &fs->inodes[inode_index];

    if (target->n_type == reg_file){
        for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++){
            //free the data block
            if (target->direct_blocks[i] != -1) {
                fs->free_list[target->direct_blocks[i]] = 1; 
                target->direct_blocks[i] = -1;
                fs->s_block[i].free_blocks += 1;

            }
        }
    } else if (target->n_type == directory) {

        // 依次遍历目标inode的direct_blocks 针对每个子inode进行递归删除操作
        // 如果子inode是目录 则构建子目录的完整路径并调用fs_rm函数进行递归删除
        // 如果子inode是file 则释放对应的数据块
        for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
            if (target->direct_blocks[i] != -1) {
                inode* child_inode = &fs->inodes[target->direct_blocks[i]];

                // einen Ordner und seinen Inhalt rekursiv löschen
                if (child_inode->n_type == directory) {
                    char child_path[NAME_MAX_LENGTH];

                    strcpy(child_path, path);
                    //appends / and the name of the child inode to the child_path string
                    strcat(child_path, "/");
                    strcat(child_path, child_inode->name);
                    fs_rm(fs, child_path); 
                } else if (child_inode->n_type == reg_file) {
                    
                    for (int j = 0; j < DIRECT_BLOCKS_COUNT; j++){
                        //free the data block
                        if (child_inode->direct_blocks[j] != -1) {
                            fs->free_list[child_inode->direct_blocks[j]] = 1; 
                            child_inode->direct_blocks[j] = -1;
                            fs->s_block[j].free_blocks += 1;
                        }
                    }
                    child_inode->n_type = free_block;
                    fs->free_list[target->direct_blocks[i]] = 1;
                }
                // if 里处理childinode之后再处理target
                target->direct_blocks[i] = -1; 
            }
        }
    }

    // 在前面已经结束了子inode为file的情况
    // 现在需要处理本inode为file和子nod为dir的情况
    target->n_type = free_block;
    for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
        //target->direct_blocks[i] = -1; 
        

        if (target->direct_blocks[i] != -1) {
            fs->free_list[target->direct_blocks[i]] = 1; 
            target->direct_blocks[i] = -1;
            fs->s_block[i].free_blocks += 1;
        }
    }
    fs->free_list[inode_index] = 1;

    // update the parent directory
    
    inode* parent = &fs->inodes[target->parent];
    for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
        if (parent->direct_blocks[i] == inode_index) {
            parent->direct_blocks[i] = -1;
            break;
        }
    }
    
    return 0;
}




int fs_import(file_system *fs, char *int_path, char *ext_path) {
    if (fs == NULL || int_path == NULL || ext_path == NULL) {
        return -1;
    }

    FILE *external_file = fopen(ext_path,"r");
    if (external_file == NULL) {
        return -1;
    }

    int inode_index = find_inode(fs, int_path);
    if (inode_index == -1 || fs->inodes[inode_index].n_type != reg_file) {
        fclose(external_file);
        return -1;
    }


    // write to the internal file
    
    char buffer[BLOCK_SIZE];
    int check_read;

    // ensure that there is space for '\0' at the end of the buffer
    // the number of elements successfully read > 0
    while ((check_read = fread(buffer, sizeof(char), BLOCK_SIZE-1, external_file)) > 0) {
        buffer[check_read] = '\0';
        int check_write = fs_writef(fs, int_path, buffer);

        if (check_write == -1) {
            fclose(external_file);
            return -1;
        }
        
    }
    

    fclose(external_file);
    return 0;
}


int fs_export(file_system *fs, char *int_path, char *ext_path) {
    if (fs == NULL || int_path == NULL || ext_path == NULL){
        return -1;
    }

    FILE *external_file = fopen(ext_path, "w");
    if (external_file == NULL) {
        return -1;
    }

    int inode_index = find_inode(fs, int_path);
    if (inode_index == -1 || fs->inodes[inode_index].n_type != reg_file) {
        return -1;
    }

    // write to the external file
    
    for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
        if (fs->inodes[inode_index].direct_blocks[i] != -1) {

            data_block *block = &fs->data_blocks[fs->inodes[inode_index].direct_blocks[i]];
            int check_write = fwrite(block->block, sizeof(char), block->size, external_file);
            
            if (check_write < block->size) {
                fclose(external_file);
                return -1;
            }
        }
    }
    
   

    fclose(external_file);
    return 0;
}
