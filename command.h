//
// Created by 邱俣涵 on 2021/10/4.
//

#ifndef FTP_SERVER_COMMAND_H
#define FTP_SERVER_COMMAND_H
#include "IO.h"
#include "core.h"
#include <arpa/inet.h>
typedef struct User User;
int handle_syst(User *user, char* sentence);
int handle_port(User *user, char*sentence);
int handle_pasv(User* user,char* sentence);
int handle_retr(User* user,char* sentence);
int handle_type(User* user,char* sentence);
int handle_stor(User* user,char* sentence);
int handle_quit(User* user,char*sentence);
int handle_list(User* user,char* sentence);
int handle_mkd(User* user,char* sentence);
int handle_pwd(User*user,char*sentence);
int handle_cwd(User*user,char* sentence);
int handle_rmd(User*user,char* sentence);
int handle_rnfr(User*user,char*sentence);
int handle_rnto(User*user, char*sentence);
#endif //FTP_SERVER_COMMAND_H
