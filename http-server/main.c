#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stddef.h>
#include <signal.h>
#include <pthread.h>
#include <sys/stat.h>

#define MAXLEN 1000
#define MAX_PATH 1024
#define CHUNK_SIZE 4096

/* Sends all len bytes, retrying on partial sends. Returns 0 on success, -1 on error. */
int send_all(int client_sock, const char *data, size_t len){
    size_t sent = 0;
    while(sent < len){
        ssize_t n = send(client_sock, data + sent, len - sent, 0);
        if(n <= 0){
            return -1;
        }
        sent += (size_t) n;
    }
    return 0;
}

void send_error(int client_sock, const char *msg){
    char message[128] = {0};
    snprintf(message, sizeof(message), "HTTP/1.1 %s\r\nContent-Length: 0\r\n\r\n", msg);
    send_all(client_sock, message, strlen(message));
}

const char *content_type(const char *filename){
    const char *ext = strrchr(filename, '.');
    if(ext == NULL){
        return "application/octet-stream";
    }
    if(strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return "text/html";
    if(strcmp(ext, ".txt") == 0) return "text/plain";
    if(strcmp(ext, ".css") == 0) return "text/css";
    if(strcmp(ext, ".js") == 0) return "text/javascript";
    if(strcmp(ext, ".json") == 0) return "application/json";
    if(strcmp(ext, ".png") == 0) return "image/png";
    if(strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    return "application/octet-stream";
}

void send_msg(int client_socket, const char *filename, int is_get){
    struct stat st;
    if(stat(filename, &st) < 0 || !S_ISREG(st.st_mode)){
        send_error(client_socket, "404 Not Found");
        return;
    }
    FILE *fp = fopen(filename, "rb");
    if(fp == NULL){
        send_error(client_socket, "403 Forbidden");
        return;
    }
    char header[256] = {0};
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %lld\r\n\r\n",
             content_type(filename), (long long) st.st_size);
    if(send_all(client_socket, header, strlen(header)) == 0 && is_get){
        char chunk[CHUNK_SIZE];
        size_t n;
        while((n = fread(chunk, 1, sizeof(chunk), fp)) > 0){
            if(send_all(client_socket, chunk, n) < 0){
                break;
            }
        }
    }
    fclose(fp);
}

/* Rejects paths that could escape the served directory. */
int is_safe_path(const char *path){
    if(path[0] != '/'){
        return 0;
    }
    if(strstr(path, "..") != NULL){
        return 0;
    }
    return 1;
}

void *handle_client_request(void *param) {
    int client_socket = *((int *) param);
    free(param);

    char buf[MAXLEN + 1];
    char method[8] = {0};
    char path[MAX_PATH] = {0};
    char http[16] = {0};

    ssize_t bytes_read = recv(client_socket, buf, MAXLEN, 0);
    if (bytes_read <= 0) {
        fprintf(stderr, "Error reading client request\n");
        close(client_socket);
        return NULL;
    }
    buf[bytes_read] = '\0';

    /* Field widths keep each token inside its buffer. */
    if(sscanf(buf, "%7s %1023s %15s", method, path, http) != 3 || strcmp(http, "HTTP/1.1") != 0){
        send_error(client_socket, "400 Bad Request");
        close(client_socket);
        return NULL;
    }

    int is_get;
    if(strcmp(method, "GET") == 0){
        is_get = 1;
    }
    else if(strcmp(method, "HEAD") == 0){
        is_get = 0;
    }
    else{
        send_error(client_socket, "501 Not Implemented");
        close(client_socket);
        return NULL;
    }

    if(!is_safe_path(path)){
        send_error(client_socket, "403 Forbidden");
        close(client_socket);
        return NULL;
    }

    if(strncmp(path, "/delay/", 7) == 0){
        int delay = atoi(path + 7);
        if(delay > 0){
            sleep((unsigned int) delay);
        }
        const char *resp = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
        send_all(client_socket, resp, strlen(resp));
    }
    else{
        send_msg(client_socket, path + 1, is_get);
    }
    close(client_socket);
    return NULL;
}

int main(int argc, char *argv[]){
    if(argc != 2){
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }
    int port = atoi(argv[1]);
    if(port <= 0 || port > 65535){
        fprintf(stderr, "Invalid port: %s\n", argv[1]);
        return 1;
    }

    /* A client that disconnects mid-response should not kill the server. */
    signal(SIGPIPE, SIG_IGN);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0){
        perror("socket");
        return 1;
    }

    int reuse = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons((unsigned short) port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(sock, (struct sockaddr *) &sa, sizeof(sa)) < 0){
        perror("bind");
        close(sock);
        return 1;
    }

    if(listen(sock, 15) < 0){
        perror("listen");
        close(sock);
        return 1;
    }
    printf("Listening on port %d\n", port);

    while(1){
        struct sockaddr_in client_sockinfo;
        socklen_t client_sa_len = sizeof(client_sockinfo);
        int client_sock = accept(sock, (struct sockaddr *) &client_sockinfo, &client_sa_len);
        if(client_sock < 0){
            perror("accept");
            continue;
        }

        char client_addr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_sockinfo.sin_addr), client_addr, INET_ADDRSTRLEN);
        printf("client_socket: %d (%s:%d)\n", client_sock, client_addr, ntohs(client_sockinfo.sin_port));

        pthread_t client_thread;
        int *p = malloc(sizeof(int));
        if(p == NULL){
            send_error(client_sock, "500 Internal Error");
            close(client_sock);
            continue;
        }
        *p = client_sock;
        if(pthread_create(&client_thread, NULL, handle_client_request, p) != 0){
            free(p);
            send_error(client_sock, "500 Internal Error");
            close(client_sock);
        }
        else{
            pthread_detach(client_thread);
        }
    }
}
