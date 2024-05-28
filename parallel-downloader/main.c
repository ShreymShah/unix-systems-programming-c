#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void curl_shrey(char *name, char *url, int max_seconds) {
    char max_sec_str[10] = {0};
    snprintf(max_sec_str, sizeof(max_sec_str), "%d", max_seconds);
    char *const args[] = {"curl", "-m", max_sec_str, "-o", name, "-s", url, NULL};

    if (max_seconds == -1) {
        url[strlen(url) - 1] = '\0';
        char *const argss[] = {"curl", "-o", name, "-s", url, NULL};
        execvp("curl", argss);
    } else {
        execvp("curl", args);
    }
}

void print_exit(int status, int line_number_arr[], pid_t id) {
    int i;
    for (i = 0; i < 20; i++) {
        if (line_number_arr[i] == id) {
            break;
        }
    }
    int line_exited = i + 1;

    if (WIFEXITED(status)) {
        int exit_status = WEXITSTATUS(status);
        if (exit_status == 0) {
            printf("process %d processing line %d exited normally\n", id, line_exited);
        } else {
            printf("process %d processing line %d terminated with exit status: %d\n", id, line_exited, exit_status);
        }
    }
}

void process_line(char *line, int line_number, int max_childs, int *num_childs, int *total_downloads, int *line_number_arr) {
    char name[256] = {0}, url[256] = {0};
    int max_seconds = -1;
    sscanf(line, "%s %s %d", name, url, &max_seconds);

    pid_t pid = fork();
    if (pid < 0) {
        printf("fork failed\n");
        return;
    }

    if (pid == 0) {
        int child_pid = getpid();
        printf("process %d processing line %d\n", child_pid, line_number);
        curl_shrey(name, url, max_seconds);
        exit(0);
    } else {
        line_number_arr[line_number - 1] = pid;
        (*num_childs)++;
        (*total_downloads)++;
    }

    if (*num_childs >= max_childs) {
        int status;
        pid_t id = waitpid(-1, &status, 0);
        print_exit(status, line_number_arr, id);
        (*num_childs)--;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Invalid number of arguments passed\n");
        return 1;
    }

    int max_childs = atoi(argv[2]);
    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    char line[256];
    int num_childs = 0, line_number = 0, total_downloads = 0;
    int line_number_arr[20] = {0};

    while (fgets(line, sizeof(line), file)) {
        line_number++;
        process_line(line, line_number, max_childs, &num_childs, &total_downloads, line_number_arr);
    }

    fclose(file);

    while (num_childs > 0) {
        int status;
        pid_t id = waitpid(-1, &status, 0);
        print_exit(status, line_number_arr, id);
        num_childs--;
    }

    return 0;
}
