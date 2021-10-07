//
// Created by 邱俣涵 on 2021/10/4.
//
//return 1表示正常进行，return 0表示错误进行
#include "IO.h"
#define MAX_DATA_SIZE 8196
int send_message(int connfd, char *buf) {
    int p = 0;
    int len= strlen(buf);
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

int send_file(int filefd, char* filename){
    /**
     * 给定文件名发送，错误状态有：文件无法打开-1;写入socket错误-2.不管如何都关闭发送口
     */
    char buf[MAX_DATA_SIZE];
    FILE * fp= fopen(filename,"rb");
    if(fp==NULL){
        close(filefd);
        return -1;
    }
    while(1){
        int n= fread(buf, sizeof(char),MAX_DATA_SIZE,fp);
        if(n==0) break;
        int m= write(filefd, buf, n);
        if(m<0){
            close(filefd);
            return -2;
        }
    }
    close(filefd);
    return 0;
}