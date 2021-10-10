//
// Created by 邱俣涵 on 2021/10/4.
//

#ifndef FTP_SERVER_CORE_H
#define FTP_SERVER_CORE_H
#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <pthread.h>

#include "command.h"
#define LISTENPORT 21

enum UserState{NOTLOGIN,LOGIN,PORTMODE,PASVMODE};
typedef struct User{
    enum UserState state;
    int connfd;//用来沟通的
    struct sockaddr_in addr;//用户的ip地址，port用
    int filefd;//用于传输数据
    FILE *fp;//文件流
    pthread_t file_thread;
    char dir[MAX_MESSAGE_SIZE];//当前所在目录.统一保留最后一个/
    char filename[MAX_MESSAGE_SIZE];//用于RNFR命令的文件名,直接存服务器文件名了
}User;
int init_server(int argc, char **argv);//set,bind,and start to listen
int init_connection(int connfd);//When user request to connect,send the 220 greeting and require Username
void* main_process(void* args);//Every thread calls this function
int handle_command(User *user, char* sentence);
int parse_arg(int argc,char** argv,char* dir,int* port);
#endif //FTP_SERVER_CORE_H
