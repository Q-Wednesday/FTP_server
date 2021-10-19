//
// Created by 邱俣涵 on 2021/10/4.
//

#ifndef FTP_SERVER_COMMAND_H
#define FTP_SERVER_COMMAND_H
#include "IO.h"
#include "core.h"
#include <arpa/inet.h>


typedef struct User User;
int handle_USER(User* user,char* sentence);
int handle_PASS(User* user,char* sentence);
int handle_SYST(User *user, char* sentence);
int handle_PORT(User *user, char*sentence);
int handle_PASV(User* user, char* sentence);
int handle_RETR(User* user, char* sentence);
int handle_TYPE(User* user, char* sentence);
int handle_STOR(User* user, char* sentence);
int handle_QUIT(User* user, char*sentence);
int handle_LIST(User* user, char* sentence);
int handle_MKD(User* user, char* sentence);
int handle_PWD(User*user, char*sentence);
int handle_CWD(User*user, char* sentence);
int handle_RMD(User*user, char* sentence);
int handle_RNFR(User*user, char*sentence);
int handle_RNTO(User*user, char*sentence);
#endif //FTP_SERVER_COMMAND_H