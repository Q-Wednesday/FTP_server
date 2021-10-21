//
// Created by 邱俣涵 on 2021/10/4.
//
#include "command.h"
#include <stdlib.h>
#include<pthread.h>

#define REQ_PASSWORD "331 Guest login ok,send your complete e-mail address as password\r\n"
#define SERVICE_UNAVAILABLE "421 Service not available, remote server has closed connection\r\n"
#define WRONG_USER "530 Wrong user name, only support anonymous.\r\n"
#define REJECT_PASS "503 USER command has not been sent.\r\n"
#define LOGIN_SUCCESS "230 Guest login ok.\r\n"
#define WRONG_FORMAT "500 Wrong format, the command cannot be recognized\r\n"
#define SYS_INFO "215 UNIX Type: L8\r\n"
#define ACCEPT_PORT "200 PORT command successful\r\n"
#define ENTER_PASV "227 Entering Passive Mode (%s,%d,%d)\r\n"
#define ACCEPT_CONNECTION "150 Opening BINARY mode data connection for %s \r\n"
#define ACCEPT_CONNECTION_ASCII "150 Opening ASCII mode data connection for %s \r\n"
#define REJECT_RETR_READFILE "451 The server had trouble reading the file\r\n"
#define ACCEPT_TYPE "200 Type set to I.\r\n"
#define WRONG_PARAM "504 The command under this parameter is not implemented\r\n"
#define GOODBYE "221 Good Bye\r\n"
#define LS_COMMAND "ls -l %s%s 2>/dev/null"
#define ACCEPT_DIR "257 \"%s\"\r\n"
#define MKD_SUCCESS "250 Create success\r\n"
#define MKD_FAILED "550 Create failed\r\n"
#define MKD_COMMAND "mkdir %s%s"
#define CWD_COMMAND "cd %s"
#define CWD_SUCCESS "250 OK \r\n"
#define CWD_FAILED "550 Failed\r\n"
#define RMD_COMMAND "rm -r %s%s"
#define RMD_SUCCESS "250 Successfully removed\r\n"
#define RMD_FAILED "550 Removal Failed\r\n"
#define REJECT_RNFR_A "550 auth\r\n"
#define REJECT_RNFR_NO_FILE "450 file not exists\r\n"
#define ACCEPT_RNFR "350 File exists\r\n"
#define RNTO_COMMAND "mv %s %s%s"
#define REJECT_RNTO "550 auth\r\n"
#define ACCEPT_RNTO "250 Renamed successfully\r\n"
#define LOCAL_ERROR "451 Requested action aborted: local error in processing.\r\n"
#define SYNTAX_ERROR "501 Syntax error in parameters or arguments.\r\n"
#define NOT_LOGIN "530 Not logged in\r\n"
#define CANNOT_OPEN_DATA_CONNECTION "425 Can't open data connection\r\n"
#define FILE_UNAVAILABLE "550 Requested action not taken; file unavailable.\r\n"
extern char rootDir[MAX_MESSAGE_SIZE];
extern char local_ip[20];

int handle_USER(User *user, char *sentence) {
    /** Handle the command USER
     * Only support anonymous
     * Whether the username is right or wrong,send 331
     * Use different states to indicate whether the username is correct
     */
    if (strcmp(sentence, "USER anonymous\r\n") == 0) {
        user->state = REQPASS;
    } else {
        user->state = WRONGUSER;
    }
    return send_message(user->connfd, REQ_PASSWORD);
}

int handle_PASS(User *user, char *sentence) {
    /** Handle the command PASS
     * Just ignore the password(Because of Anonymity)
     * Send 230 if User name is right
     * Send 530 if wrong
     * Send 503 if the previous command is not USER
     */
    if (user->state == WRONGUSER) {
        user->state = NOTLOGIN;
        return send_message(user->connfd, WRONG_USER);
    }
    if (user->state != REQPASS) {
        user->state = NOTLOGIN;
        return send_message(user->connfd, REJECT_PASS);
    }
    user->state = LOGIN;
    return send_message(user->connfd, LOGIN_SUCCESS);
}

int handle_SYST(User *user, char *sentence) {
    /** Handle the command SYST
     * Send 215 if the format of the command is right
     * Otherwise send 500
     */
    if (strcmp(sentence, "SYST\r\n") == 0) {
        return send_message(user->connfd, SYS_INFO);
    }
    return send_message(user->connfd, WRONG_FORMAT);
}

int handle_TYPE(User *user, char *sentence) {
    /** Handle the command TYPE
     * Only support TYPE I
     * Send 200 if the parameter is right
     * Otherwise send 504
     */
    if (sentence[5] == 'I') {
        return send_message(user->connfd, ACCEPT_TYPE);
    }
    return send_message(user->connfd, WRONG_PARAM);
}

int handle_PORT(User *user, char *sentence) {
    /** Handle the command PORT
     * Accept with 200
     */

    //Drop the connection already made
    if (user->state == PORTMODE || user->state == PASVMODE) {
        close(user->filefd);
        user->state = LOGIN;
    }
    //If the user did not login, tell the user
    if (user->state != LOGIN) {
        send_message(user->connfd, NOT_LOGIN);
        return -1;
    }
    //First,find the address sent by client and parse it.
    int p = 0;
    while (sentence[p] < '0' || sentence[p] > '9') {
        p++;
    }
    //If cannot be parsed,tell the client
    if (parse_ip(user, sentence + p) != 0) {
        return send_message(user->connfd, SYNTAX_ERROR);
    }
    //Create a socket but do not connect immediately
    //If cannot create,tell the client
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("Error socket(): %s(%d)\n", strerror(errno), errno);
        return send_message(user->connfd, CANNOT_OPEN_DATA_CONNECTION);
    }
    user->filefd = sockfd;
    user->state = PORTMODE;
    return send_message(user->connfd, ACCEPT_PORT);
}

int handle_PASV(User *user, char *sentence) {
    /** Handle the command PASV
     *  accept with 227 and tell the client server's ip and port
     */
    //Drop the connection already made
    if (user->state == PASVMODE || user->state == PORTMODE) {
        close(user->filefd);
        user->state = LOGIN;
    }
    //If the user did not login, close the connection
    if (user->state != LOGIN) {
        send_message(user->connfd, NOT_LOGIN);
        return -1;
    }

    //set server's IP address
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    int random_port = rand() % 45535 + 20000;
    char message[MAX_MESSAGE_SIZE];
    addr.sin_family = AF_INET;
    addr.sin_port = htons(random_port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);    //listen on "0.0.0.0"
    int listenfd;
    //When any error occured, tell the client
    if (listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) ||
                   bind(listenfd, (struct sockaddr *) &addr, sizeof(addr)) ||
                   listen(listenfd, 10)) {
        printf("Error : %s(%d)\n", strerror(errno), errno);
        return send_message(user->connfd, CANNOT_OPEN_DATA_CONNECTION);
    }

    sprintf(message, ENTER_PASV, local_ip, random_port / 256, random_port % 256);
    send_message(user->connfd, message);
    if ((user->filefd = accept(listenfd, NULL, NULL)) == -1) {
        printf("Error accept(): %s(%d)\n", strerror(errno), errno);
        return send_message(user->connfd, CANNOT_OPEN_DATA_CONNECTION);
    }
    user->state = PASVMODE;
    return 0;
}

int handle_RETR(User *user, char *sentence) {
    /** Handle the command RETR
     *  This function will create a new thread for file transfer
     */

    //First check the connection
    if (connect_filefd(user, sentence) != 0) {
        return 0;
    }

    //Then tell the user the connection was accepted
    char filename[MAX_DATA_SIZE], message[MAX_MESSAGE_SIZE];
    size_t len = strlen(sentence);
    sentence[len - 2] = '\0';//drop \r\n
    sprintf(filename, "%s%s/%s", rootDir, user->dir, sentence + 5);
    sprintf(message, ACCEPT_CONNECTION, sentence + 5);
    send_message(user->connfd, message);
    user->fp = fopen(filename, "rb");
    //Handle the error while opening file
    if (!user->fp) {
        close(user->filefd);
        return send_message(user->connfd, REJECT_RETR_READFILE);
    }
    if(pthread_create(&user->file_thread, NULL, &send_file, user)){
        return send_message(user->connfd,LOCAL_ERROR);
    }

    return 0;
}


int handle_STOR(User *user, char *sentence) {
    /** Handle the command STOR
    *  This function will create a new thread for file transfer
    */
    //First check the connection
    if (connect_filefd(user, sentence) != 0) {
        return 0;
    }
    //Then tell the user the connection was accepted
    char filename[MAX_DATA_SIZE], message[MAX_MESSAGE_SIZE];
    size_t len = strlen(sentence);
    sentence[len - 2] = '\0';//drop \r\n
    sprintf(filename, "%s%s/%s", rootDir, user->dir, sentence + 5);
    sprintf(message, ACCEPT_CONNECTION, sentence + 5);
    send_message(user->connfd, message);
    user->fp = fopen(filename, "wb");
    if (!user->fp) {
        close(user->filefd);
        return send_message(user->connfd, REJECT_RETR_READFILE);
    }
    if(pthread_create(&user->file_thread, NULL, &receive_file, user)){
        return send_message(user->connfd,LOCAL_ERROR);
    }

    return 0;
}

int handle_QUIT(User *user, char *sentence) {
    /** Handle the command QUIT
     * return -1 and the main_process will release the user
     */
    close(user->filefd);
    fclose(user->fp);
    send_message(user->connfd, GOODBYE);
    return -1;
}


int handle_LIST(User *user, char *sentence) {
    /** Handle the command LIST
     * Use a pipe to get the output of command "ls -l" in shell
     * If the parameter is parent directory of the root,send 550
     */

    //First check the connection
    if (connect_filefd(user, sentence) != 0) {
        return 0;
    }

    char command[MAX_DATA_SIZE], message[MAX_MESSAGE_SIZE];
    //Generate the command that will be executed in shell
    if (strlen(sentence) <= 6) {
        sprintf(command, LS_COMMAND, rootDir, user->dir);
    } else {
        char user_path_parsed[MAX_MESSAGE_SIZE];
        if (parse_dir(user_path_parsed, sentence + 5, user)) {
            return send_message(user->connfd, FILE_UNAVAILABLE);
        }
        sprintf(command, LS_COMMAND, rootDir, user_path_parsed);
    }

    sprintf(message, ACCEPT_CONNECTION_ASCII, "ls");
    send_message(user->connfd, message);
    user->fp = popen(command, "rb");
    if (!user->fp) {
        close(user->filefd);
        return send_message(user->connfd, REJECT_RETR_READFILE);
    }
    if(pthread_create(&user->file_thread, NULL, &send_file, user)){
        return send_message(user->connfd,LOCAL_ERROR);
    }

    return 0;

}

int handle_MKD(User *user, char *sentence) {
    //TODO:处理创建出错的情况.不允许出现../
    char command[MAX_DATA_SIZE], user_path_parsed[MAX_MESSAGE_SIZE];//message[MAX_MESSAGE_SIZE],
    if (parse_dir(user_path_parsed, sentence + 4, user)) {
        return send_message(user->connfd, MKD_FAILED);
    }

    sprintf(command, MKD_COMMAND, rootDir, user_path_parsed);
    if (system(command) == 0) {
        return send_message(user->connfd, MKD_SUCCESS);
    }

    return send_message(user->connfd, MKD_FAILED);
}

int handle_PWD(User *user, char *sentence) {
    char message[MAX_DATA_SIZE];
    sprintf(message, ACCEPT_DIR, user->dir);
    return send_message(user->connfd, message);
}

int handle_CWD(User *user, char *sentence) {

    char command[MAX_DATA_SIZE], full_path[MAX_MESSAGE_SIZE * 2],
            user_path_parsed[MAX_MESSAGE_SIZE];

    if (parse_dir(user_path_parsed, sentence + 4, user)) {
        return send_message(user->connfd, CWD_FAILED);
    }

    sprintf(full_path, "%s%s", rootDir, user_path_parsed);
    sprintf(command, CWD_COMMAND, full_path);
    if (system(command) == 0) {
        strcpy(user->dir, user_path_parsed);
        return send_message(user->connfd, CWD_SUCCESS);
    }
    return send_message(user->connfd, CWD_FAILED);
}

int handle_RMD(User *user, char *sentence) {
    //TODO:处理权限

    char command[MAX_DATA_SIZE], user_path_parsed[MAX_MESSAGE_SIZE];
    if (parse_dir(user_path_parsed, sentence + 4, user)) {
        return send_message(user->connfd, RMD_FAILED);
    }
    sprintf(command, RMD_COMMAND, rootDir, user_path_parsed);
    if (system(command) == 0) {
        return send_message(user->connfd, RMD_SUCCESS);
    }
    return send_message(user->connfd, RMD_FAILED);
}

int handle_RNFR(User *user, char *sentence) {
    //先检测文件是否存在
    char user_path_parsed[MAX_MESSAGE_SIZE * 2], command[MAX_DATA_SIZE];
    if (parse_dir(user_path_parsed, sentence + 5, user)) {
        return send_message(user->connfd, REJECT_RNFR_A);
    }
    sprintf(user->filename, "%s%s", rootDir, user_path_parsed);
    sprintf(command, "ls %s", user->filename);
    if (system(command)) {
        return send_message(user->connfd, REJECT_RNFR_NO_FILE);
    }
    return send_message(user->connfd, ACCEPT_RNFR);
}

int handle_RNTO(User *user, char *sentence) {
    char user_path_parsed[MAX_MESSAGE_SIZE * 2], command[MAX_DATA_SIZE];
    if (parse_dir(user_path_parsed, sentence + 5, user)) {
        return send_message(user->connfd, REJECT_RNTO);
    }
    sprintf(command, RNTO_COMMAND, user->filename, rootDir, user_path_parsed);
    if (system(command)) {
        return send_message(user->connfd, REJECT_RNTO);
    }
    return send_message(user->connfd, ACCEPT_RNTO);
}