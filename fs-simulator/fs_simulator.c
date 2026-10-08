#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_INODES 1024
#define NAME_LEN 32

/* Opens the file that stores the given inode. */
FILE *open_inode(int inode, const char *mode){
    char str[32] = {0};
    snprintf(str, sizeof(str), "%d", inode);
    FILE *f = fopen(str, mode);
    if(f == NULL){
        perror(str);
    }
    return f;
}

/* Reads the next directory entry. Returns 1 on success, 0 at end of directory. */
int read_entry(FILE *f, int *inode_num, char name[NAME_LEN]){
    if(fread(inode_num, sizeof(int), 1, f) != 1){
        return 0;
    }
    if(fread(name, sizeof(char), NAME_LEN, f) != NAME_LEN){
        return 0;
    }
    name[NAME_LEN - 1] = '\0';
    return 1;
}

void write_entry(FILE *f, int inode_num, const char *name){
    char padded[NAME_LEN] = {0};
    strncpy(padded, name, NAME_LEN - 1);
    fwrite(&inode_num, sizeof(int), 1, f);
    fwrite(padded, sizeof(char), NAME_LEN, f);
}

/* Returns 1 if the directory already has an entry with this name. */
int name_exists(int dir, const char *name){
    FILE *f = open_inode(dir, "rb");
    if(f == NULL){
        return 0;
    }
    int inode_num;
    char entry_name[NAME_LEN];
    int found = 0;
    while(read_entry(f, &inode_num, entry_name)){
        if(strcmp(entry_name, name) == 0){
            found = 1;
            break;
        }
    }
    fclose(f);
    return found;
}

void ls(int curr_dir){
    FILE *f = open_inode(curr_dir, "rb");
    if(f == NULL){
        return;
    }
    int inode_num;
    char name[NAME_LEN];
    while(read_entry(f, &inode_num, name)){
        printf("%d %s\n", inode_num, name);
    }
    fclose(f);
}

int cd(int curr_dir, const char *dir_name, const char inode_list[]){
    FILE *f = open_inode(curr_dir, "rb");
    if(f == NULL){
        return curr_dir;
    }
    int inode_num;
    char name[NAME_LEN];
    while(read_entry(f, &inode_num, name)){
        if(strcmp(name, dir_name) == 0 && inode_num >= 0 && inode_num < MAX_INODES
           && inode_list[inode_num] == 'd'){
            fclose(f);
            return inode_num;
        }
    }
    fclose(f);
    printf("Directory %s not found\n", dir_name);
    return curr_dir;
}

/* Records a new inode in inodes_list, in memory and on disk. */
void add_inode(int inode, char type, char inode_list[]){
    FILE *f = fopen("inodes_list", "ab");
    if(f == NULL){
        perror("inodes_list");
        return;
    }
    fwrite(&inode, sizeof(int), 1, f);
    fwrite(&type, sizeof(char), 1, f);
    fclose(f);
    inode_list[inode] = type;
}

/* Adds a name -> inode entry to the parent directory. */
void add_to_dir(int dir, int inode, const char *name){
    FILE *f = open_inode(dir, "ab");
    if(f == NULL){
        return;
    }
    write_entry(f, inode, name);
    fclose(f);
}

void make_dir(int prev_dir, const char *new_dir_name, int *max_inode, char inode_list[]){
    if(name_exists(prev_dir, new_dir_name)){
        printf("Directory %s already present, please choose another name\n", new_dir_name);
        return;
    }
    int new_inode = *max_inode + 1;

    /* A new directory starts with "." and ".." entries. */
    FILE *f = open_inode(new_inode, "wb");
    if(f == NULL){
        return;
    }
    write_entry(f, new_inode, ".");
    write_entry(f, prev_dir, "..");
    fclose(f);

    add_inode(new_inode, 'd', inode_list);
    add_to_dir(prev_dir, new_inode, new_dir_name);
    *max_inode = new_inode;
}

void touch(int prev_dir, const char *new_file_name, int *max_inode, char inode_list[]){
    if(name_exists(prev_dir, new_file_name)){
        printf("File name '%s' already exists.\n", new_file_name);
        return;
    }
    int new_inode = *max_inode + 1;

    FILE *f = open_inode(new_inode, "wb");
    if(f == NULL){
        return;
    }
    write_entry(f, new_inode, new_file_name);
    fclose(f);

    add_inode(new_inode, 'f', inode_list);
    add_to_dir(prev_dir, new_inode, new_file_name);
    *max_inode = new_inode;
}

/* Loads inodes_list into memory. Returns the highest inode number, or -1 on error. */
int load_inodes(char inode_list[]){
    FILE *file = fopen("inodes_list", "rb");
    if(file == NULL){
        perror("inodes_list");
        return -1;
    }
    int max_inode = 0;
    int i;
    char type;
    while(fread(&i, sizeof(int), 1, file) == 1 && fread(&type, sizeof(char), 1, file) == 1){
        if(i < 0 || i >= MAX_INODES){
            fprintf(stderr, "Skipping invalid inode number %d in inodes_list\n", i);
            continue;
        }
        inode_list[i] = type;
        if(i > max_inode){
            max_inode = i;
        }
    }
    fclose(file);
    return max_inode;
}

int main(int argc, char *argv[]){
    char inode_list[MAX_INODES] = {0};

    if(argc < 2){
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        return 1;
    }
    if(chdir(argv[1]) != 0){
        perror(argv[1]);
        return 1;
    }
    int max_inode = load_inodes(inode_list);
    if(max_inode < 0){
        return 1;
    }
    int curr_dir = 0;

    char command[256];
    while(1){
        printf("> ");
        fflush(stdout);
        if(fgets(command, sizeof(command), stdin) == NULL){
            printf("\n");
            break;
        }
        if(strchr(command, '\n') == NULL && !feof(stdin)){
            /* Discard the rest of an over-long line. */
            int c;
            while((c = getchar()) != '\n' && c != EOF){
            }
            printf("Command too long\n");
            continue;
        }

        char first[256] = {0};
        char second[256] = {0};
        char extra[256] = {0};
        int count = sscanf(command, "%255s %255s %255s", first, second, extra);

        if(count <= 0){
            continue;
        }
        if(count > 2){
            printf("Command invalid\n");
            continue;
        }

        if(strcmp(first, "ls") == 0){
            ls(curr_dir);
        }
        else if(strcmp(first, "exit") == 0){
            break;
        }
        else if(strcmp(first, "cd") == 0 || strcmp(first, "mkdir") == 0 || strcmp(first, "touch") == 0){
            if(count < 2){
                printf("%s needs a name\n", first);
                continue;
            }
            if(strlen(second) >= NAME_LEN){
                printf("Name too long (max %d characters)\n", NAME_LEN - 1);
                continue;
            }
            if(strcmp(first, "cd") == 0){
                curr_dir = cd(curr_dir, second, inode_list);
            }
            else if(max_inode >= MAX_INODES - 1){
                printf("Maximum number of inodes reached!\n");
            }
            else if(strcmp(first, "mkdir") == 0){
                make_dir(curr_dir, second, &max_inode, inode_list);
            }
            else{
                touch(curr_dir, second, &max_inode, inode_list);
            }
        }
        else{
            printf("Command does not exist\n");
        }
    }
    return 0;
}
