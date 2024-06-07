#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <stddef.h>
#include <signal.h>
#include <pthread.h>
#include <sys/stat.h>

#define MAXLEN 1000

void slice(const char* str, int start, int end, char* return_str) {
    int length = end - start;
    int i;
    for (i = 0; i < length; i++) {
        return_str[i] = str[start + i];
    }

    return_str[length] = '\0';

}

void send_error(int client_sock, char* msg){
    char message[1052] = {0};
    snprintf(message,1052,"HTTP/1.1 %s\r\n\r\n",msg);
    send(client_sock,message,strlen(message),0);

}

void send_msg(int client_socket, char *filename, char is_get){
    struct stat st;
    if(stat(filename,&st) < 0){
        send_error(client_socket, "404 Not Found");
        return;
    }
    FILE *fp = fopen(filename,"r");
    char msg[1052] = {0};
    int size = st.st_size;
    snprintf(msg,1052, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n",size);
    send(client_socket,msg,strlen(msg),0);
    if(is_get == 'y'){
        char c[2];
        c[1] = '\0';
        while((c[0]=fgetc(fp)) != EOF){
            send(client_socket,c,strlen(c),0);
        }
    }

    fclose(fp);

}

void *handle_client_request(void *param) {
    int client_socket = *((int *) param);
    char buf[MAXLEN+1];
    char is_get='n';
    char filename[1052] = {0};
    char head_get[5] = {0};
    char delay_filename1[15] = {0};
    char http[15] = {0};
    int delay = 0;
    memset(buf, 0, MAXLEN+1);
    ssize_t bytes_read = recv(client_socket, buf, sizeof(buf), 0);
    if (bytes_read <= 0) {
        printf("Error reading client request\n");
        close(client_socket);
        free((int *)param);
        return NULL;
    }

    buf[bytes_read-2] = '\0';
    sscanf(buf, "%s %s %s",head_get, delay_filename1, http);

    if(strcmp(http, "HTTP/1.1") !=0 ){
        send_error(client_socket,"400 Bad Request");
        close(client_socket);
        free((int *)param);
        return NULL;
    }

    if(strcmp(head_get,"GET") == 0){
        is_get = 'y';
    }
    else if(buf[0] == 'H'){
        ;
    }
    else{
        send_error(client_socket,"501 Not Implemented");
        close(client_socket);
        free((int *)param);
        return NULL;
    }
    char delay_slice[10] = {0};
    slice(delay_filename1,0,7,delay_slice);
    if(strcmp(delay_slice,"/delay/") == 0){
        delay = atoi(delay_filename1+7);
        sleep(delay);
        char *resp = "HTTP/1.1 200 OK \r\n\r\n";
        send(client_socket,resp,strlen(resp),0);
        close(client_socket);
        free((int *)param);
        return NULL;
    }
    else{ 
        slice(delay_filename1,1,strlen(delay_filename1),filename);
        send_msg(client_socket,filename,is_get);
    }
    close(client_socket);
    free((int *)param);
    return NULL;

}

int main(int argc, char *argv[]){
    if(argc!=2){
        printf("Invalid number of arguements\n");
        return 0;
    }
    int port = atoi(argv[1]);
    int sock, client_sock;
    
    struct sockaddr_in sa, client_sockinfo;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if(sock<0){
        printf("Error in socket\n");
        return 0;
    }
    
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    connect(sock, (struct sockaddr *) &sa, sizeof(sa));
    printf("Connected to socket %d\n",sock);
    int b = bind(sock, (struct sockaddr*)&sa, sizeof(sa));

    if(b<0){
        printf("Error in bind\n");
        return 0;
    }

    int l = listen(sock,15);
    if(l<0){
        printf("Error in listen\n");
        return 0;
    }

    while(1){
        socklen_t client_sa_len;
        client_sock = accept(sock, (struct sockaddr *) &client_sockinfo, &client_sa_len);
        char client_addr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(sa.sin_addr), client_addr, INET_ADDRSTRLEN);  // convert client IP address into printable string
        printf("client_socket: %d (%s:%d)\n", client_sock, client_addr, ntohs(sa.sin_port));

        pthread_t client_thread;
        int *p = (int *) malloc(sizeof(int));
        *p = client_sock;
        int check = pthread_create(&client_thread, NULL, handle_client_request, p);
        if(check!=0){
            send_error(client_sock,"500 Internal Error");
            close(client_sock);
        }
        else{
            pthread_detach(client_thread);
        }
    }
}