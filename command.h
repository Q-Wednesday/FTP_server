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
#endif //FTP_SERVER_COMMAND_H
