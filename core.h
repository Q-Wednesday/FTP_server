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
#define MAX_CONNECTION 2
// User's States.
enum UserState{NOTLOGIN,REQPASS,LOGIN,PORTMODE,PASVMODE};
typedef struct User{
    int userNo;//The identifier assigned to the user, indicating the user's position in the array
    enum UserState state;
    int connfd;//Used for Message
    struct sockaddr_in addr;//User's IP,used for PORT
    int filefd;//used for File
    FILE *fp;//Pointer of Opened File
    pthread_t file_thread;//The thread used for file transfer
    char dir[MAX_MESSAGE_SIZE];//User's workDir in FTP server.Can't end with '/' except the root. e.g. "/home"
    char filename[MAX_MESSAGE_SIZE];//used for RNFR
}User;
int init_server(int argc, char **argv);//set,bind,and start to listen
void* main_process(void* args);//Every user has one
int handle_command(User *user, char* sentence);//Handle the command
int parse_arg(int argc, char **argv, int *port);//Parse the arg
void get_local_ip(char *buf);
#endif //FTP_SERVER_CORE_H
