#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/wait.h>

void curl_shrey(char *name, char *url, int max_seconds){
    char max_sec_str[10] = {0};
    snprintf(max_sec_str,sizeof(max_sec_str),"%d",max_seconds);
    char *const args[] = {"curl", "-m", max_sec_str, "-o", name,"-s",url,NULL};
    if(max_seconds==-1){
        url[strlen(url)-1] = '\0';
        char *const argss[] = {"curl", "-o", name,"-s",url,NULL};
        execvp("curl",argss);
    }
    else{
        execvp("curl",args);
    }

}

void print_exit(int status, int line_number_arr[], pid_t id){
    int i=0;
    int line_exited;
    while(i<20){
        if(line_number_arr[i] == id){
            line_exited = i+1; 
            break; 
        }
            i++;
    }

        if(WIFEXITED(status)){
            int exit_status = WEXITSTATUS(status);
            if(exit_status==0){
                printf("process %d processing line %d exited normally\n",id,line_exited);
            }
            else{
                printf("process %d processing line %d terminated with exit status: %d\n",id,line_exited,exit_status);
            }
        }    
}

int main(int argc, char *argv[]){
    if(argc<3){
        printf("Invalid number of arguments passed\n");
        return 0;
    }
    int max_childs = atoi(argv[2]);
    char *file_name = argv[1];
    FILE *file = fopen(file_name,"r");
    char line[256] = {0};
    int num_childs = 0;
    int line_number = 0;
    int total_downloads = 0;
    int total_downloads_printed = 0;

    int line_number_arr[20] = {0};

    while(fgets(line,sizeof(line),file)){
        line_number++;
        if(num_childs>=max_childs){
            int status;
            pid_t id = waitpid(-1, &status, 0);
            print_exit(status,line_number_arr,id);
            total_downloads_printed++;
            num_childs--;
        }
        int i=0;
        char word[256] = {0};
        char empty_string[1] = {0};
        char name[256] = {0};
        char url[256] = {0};
        int max_seconds = -1;

        int j =0;
        int k=1;

        for(;i<=strlen(line);i++){
            if(line[i] == ' ' || line[i] == '\0'){
                word[j] = '\0';
                j=0;
                if(k==1){
                    strcpy(name,word);
                }
                else if(k ==2){
                    strcpy(url,word);
                }
                else if(k==3){
                    max_seconds = atoi(word);
                }
                k++;
                strcpy(word,empty_string);
            }
            else{
                word[j] = line[i];
                j++;
            }
            
        }

        pid_t pid = fork();
        if(pid<0){
            printf("fork failed\n");
        }
        
        else if(pid == 0){
            int child_pid = getpid();
            printf("process %d processing line %d\n",child_pid,line_number);
            curl_shrey(name,url,max_seconds);
        }

        else{
            line_number_arr[line_number-1] = pid;
            num_childs++;
            total_downloads++;
        }
    }

    int j=1;
    while(j<=(total_downloads-total_downloads_printed)){
            int status;
            pid_t id = waitpid(-1, &status, 0);
            print_exit(status,line_number_arr,id);
        j++;
    }

    return 1;
}