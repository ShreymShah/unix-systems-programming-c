#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define FIELD_LEN 256

/* Maps each child pid to the input line it is downloading. */
struct child {
    pid_t pid;
    int line_number;
};

/* Replaces the current (child) process with curl. Only returns if exec fails. */
void run_curl(char *name, char *url, int max_seconds){
    char max_sec_str[16] = {0};
    snprintf(max_sec_str, sizeof(max_sec_str), "%d", max_seconds);
    char *const args_timeout[] = {"curl", "-m", max_sec_str, "-o", name, "-s", url, NULL};
    char *const args[] = {"curl", "-o", name, "-s", url, NULL};
    if(max_seconds > 0){
        execvp("curl", args_timeout);
    }
    else{
        execvp("curl", args);
    }
}

void print_exit(int status, struct child *children, int num_children, pid_t id){
    int line_exited = -1;
    for(int i = 0; i < num_children; i++){
        if(children[i].pid == id){
            line_exited = children[i].line_number;
            break;
        }
    }

    if(WIFEXITED(status)){
        int exit_status = WEXITSTATUS(status);
        if(exit_status == 0){
            printf("process %d processing line %d exited normally\n", (int) id, line_exited);
        }
        else{
            printf("process %d processing line %d terminated with exit status: %d\n", (int) id, line_exited, exit_status);
        }
    }
    else if(WIFSIGNALED(status)){
        printf("process %d processing line %d killed by signal %d\n", (int) id, line_exited, WTERMSIG(status));
    }
}

/* Waits for any one child and reports how it exited. */
void wait_for_child(struct child *children, int num_children){
    int status;
    pid_t id = waitpid(-1, &status, 0);
    if(id > 0){
        print_exit(status, children, num_children, id);
    }
}

int main(int argc, char *argv[]){
    if(argc != 3){
        fprintf(stderr, "Usage: %s <url_file> <max_processes>\n", argv[0]);
        return 1;
    }
    int max_childs = atoi(argv[2]);
    if(max_childs <= 0){
        fprintf(stderr, "max_processes must be a positive number\n");
        return 1;
    }
    FILE *file = fopen(argv[1], "r");
    if(file == NULL){
        perror(argv[1]);
        return 1;
    }

    struct child *children = NULL;
    int num_children = 0;
    int capacity = 0;
    int running = 0;
    int line_number = 0;
    char line[1024];

    while(fgets(line, sizeof(line), file)){
        line_number++;
        line[strcspn(line, "\r\n")] = '\0';

        char name[FIELD_LEN] = {0};
        char url[FIELD_LEN] = {0};
        int max_seconds = -1;
        int fields = sscanf(line, "%255s %255s %d", name, url, &max_seconds);
        if(fields < 2){
            if(line[0] != '\0'){
                fprintf(stderr, "skipping malformed line %d\n", line_number);
            }
            continue;
        }

        if(running >= max_childs){
            wait_for_child(children, num_children);
            running--;
        }

        if(num_children == capacity){
            capacity = capacity == 0 ? 16 : capacity * 2;
            struct child *grown = realloc(children, (size_t) capacity * sizeof(struct child));
            if(grown == NULL){
                perror("realloc");
                break;
            }
            children = grown;
        }

        /* Flush so buffered output is not duplicated into the child. */
        fflush(stdout);
        pid_t pid = fork();
        if(pid < 0){
            perror("fork");
        }
        else if(pid == 0){
            printf("process %d processing line %d\n", (int) getpid(), line_number);
            fflush(stdout);
            run_curl(name, url, max_seconds);
            perror("execvp curl");
            _exit(127);
        }
        else{
            children[num_children].pid = pid;
            children[num_children].line_number = line_number;
            num_children++;
            running++;
        }
    }
    fclose(file);

    while(running > 0){
        wait_for_child(children, num_children);
        running--;
    }

    free(children);
    return 0;
}
