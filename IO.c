//
// Created by 邱俣涵 on 2021/10/4.
//
#include "IO.h"
#define ACCECPT_FILE_TRANS "226 Transfer complete\r\n"
#define CONNECTION_CLOSED "426 Connection closed; transfer aborted.\r\n"
int send_message(int connfd, char *buf) {
    ssize_t p = 0;
    size_t len= strlen(buf);
    while (p < len) {
        ssize_t n = write(connfd, buf + p, len - p);
        if (n <= 0) {
            return -1;
        } else {
            p += n;
        }
    }
    return 0;
}

int receive_message(int connfd,char* buf){
    ssize_t p=0;
    while (1) {
        ssize_t n = read(connfd, buf + p, MAX_MESSAGE_SIZE - p);
        if (n < 0) {
            close(connfd);
            return -1;
        } else if (n == 0) {
            break;
        } else {
            p += n;
            if (buf[p - 1] == '\n') {
                break;
            }
        }
    }
    buf[p]='\0';
    return 0;
}

void* send_file(void *args){

    User* user=(User*)args;
    char buf[MAX_DATA_SIZE];
    size_t n=1;
    ssize_t m;
    while(n!=0){
        n= fread(buf, sizeof(char),MAX_DATA_SIZE,user->fp);
        m= send(user->filefd,buf,n,0);
        if(m<0){
            send_message(user->connfd,CONNECTION_CLOSED);
            close(user->filefd);
            user->state=LOGIN;
            return NULL;
        }
    }
    close(user->filefd);
    fclose(user->fp);
    user->state=LOGIN;
    send_message(user->connfd,ACCECPT_FILE_TRANS);
    return NULL;
}
void* receive_file(void* args){

    User* user=(User*)args;
    char buf[MAX_DATA_SIZE]={0};

    ssize_t n=MAX_DATA_SIZE;
    while (1){
        n= recv(user->filefd,buf,MAX_DATA_SIZE,MSG_WAITALL);
        if(n<0){
            close(user->filefd);
            fclose(user->fp);
            send_message(user->connfd,CONNECTION_CLOSED);
            user->state=LOGIN;
            return NULL;
        }else if(n==0){
            break;
        }
        fwrite(buf, sizeof(char),n,user->fp);
    }
    user->state = LOGIN;
    fclose(user->fp);
    close(user->filefd);
    send_message(user->connfd, ACCECPT_FILE_TRANS);
    return NULL;
}