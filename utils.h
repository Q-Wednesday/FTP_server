//
// Created by 邱俣涵 on 2021/10/20.
//

#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H
#define MAX_DATA_SIZE 8196
#define MAX_MESSAGE_SIZE 1024
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <arpa/inet.h>
// User's States.
enum UserState{NOTLOGIN,WRONGUSER,REQPASS,LOGIN,PORTMODE,PASVMODE};
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
int parse_ip(User *user, const char *sentence);
int connect_filefd(User *user, char *sentence);
int parse_dir(char *user_path_parsed, char *source, User *user);
int parse_arg(int argc, char **argv, int *port, char *root);//Parse the arg
void get_local_ip(char *buf);
#endif //SERVER_UTILS_H
