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
#include  <unistd.h>
#include <signal.h>
#include "command.h"
#define LISTENPORT 21
#define MAX_CONNECTION 10
typedef struct ServerParams{
    int argc;
    char** argv;
}ServerParams;
void* init_server(void* args);//set,bind,and start to listen
void* main_process(void* args);//Every user has one
int handle_command(User *user, char* sentence);//Handle the command
#endif //FTP_SERVER_CORE_H
