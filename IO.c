//
// Created by 邱俣涵 on 2021/10/4.
//
//return 1表示正常进行，return 0表示错误进行
#include "IO.h"
#include "core.h"

int send_message(int connfd, char *buf) {
    int p = 0;
    size_t len= strlen(buf);
    while (p < len) {
        ssize_t n = write(connfd, buf + p, len - p);
        printf("send %ld\n",n);
        if (n <= 0) {
            printf("Error write(): %s(%d)\n", strerror(errno), errno);
            return -1;
        } else {
            p += n;
        }
    }
    printf("send %s",buf);
    return 0;
}

int receive_message(int connfd,char* buf,int* len){
    int p=0;
    while (1) {
        int n = read(connfd, buf + p, MAX_MESSAGE_SIZE - p);
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

void* send_file(void *args){
    /**
     * 给定文件名发送，错误状态有：文件无法打开-1;写入socket错误-2.不管如何都关闭发送口
     */
    User* user=(User*)args;
    char buf[MAX_DATA_SIZE];
    size_t n=1;
    ssize_t m=-1;
    while(n!=0){
        n= fread(buf, sizeof(char),MAX_DATA_SIZE,user->fp);

        m= write(user->filefd, buf, n);
        if(m<0){
            //TODO:做异常处理
            close(user->filefd);
            return NULL;
        }
    }
    close(user->filefd);
    fclose(user->fp);
    user->state=LOGIN;
    send_message(user->connfd,"226 Transfer complete\r\n");
    //int i= recv(filefd,buf,MAX_DATA_SIZE,MSG_PEEK);
    return NULL;
}
int receive_file(int filefd,char* filename){
    /**
     *
     */
    //TODO：处理文件名重复的情况等等.传输有错误，那边已经发完了这边还在等待输入
    char buf[MAX_DATA_SIZE]={0};
    FILE *fp= fopen(filename,"wb");
    if(fp==NULL){
        close(filefd);
        return -1;
    }
    ssize_t n=MAX_DATA_SIZE;
    while (n==MAX_DATA_SIZE){
        n= recv(filefd,buf,MAX_DATA_SIZE,MSG_WAITALL);
        if(n<0){
            close(filefd);
            return -2;
        }else if(n==0){
            break;
        }
        fwrite(buf, sizeof(char),n,fp);

    }
    fclose(fp);
    close(filefd);
    return 0;

}