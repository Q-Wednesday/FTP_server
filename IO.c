//
// Created by 邱俣涵 on 2021/10/4.
//
//return 1表示正常进行，return 0表示错误进行
#include "IO.h"
int send_message(int connfd,char* buf,unsigned long len){
    int p = 0;
    while (p < len) {
        int n = write(connfd, buf + p, len - p);
        printf("send %d\n",n);
        if (n < 0) {
            printf("Error write(): %s(%d)\n", strerror(errno), errno);
            return -1;
        } else {
            p += n;
        }
    }
    return 0;
}

int receive_message(int connfd,char* buf,int* len){
    int p=0;
    while (1) {
        int n = read(connfd, buf + p, 8191 - p);
        printf("receive: %d\n",n);
        if (n < 0) {
            printf("Error read(): %s(%d)\n", strerror(errno), errno);
            close(connfd);
            continue;
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
    printf("content received:%s\n",buf);
    printf("len:%lu\n", strlen(buf));
    *len=p;
    return 0;
}