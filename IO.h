//
// Created by 邱俣涵 on 2021/10/4.
//

#ifndef FTP_SERVER_IO_H
#define FTP_SERVER_IO_H
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
int send_message(int connfd,char* buf,int len);
int receive_message(int connfd,char* buf,int* len);
#endif //FTP_SERVER_IO_H
