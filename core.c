//
// Created by 邱俣涵 on 2021/10/4.
//
#include "core.h"
#define GREETING "220 Anonymous FTP server ready.\r\n"
#define SERVICE_UNAVAILABLE "421 Service not available, remote server has closed connection\r\n"
#define MAX_CONNECTIONS_REACHED "421 The maximum number of connections has been reached\r\n"
char rootDir[MAX_MESSAGE_SIZE] = "/tmp";
char local_ip[20];
int running = 1;//
int num_threads = 0;//Number of running threads




void* init_server(void* args) {
    /** Get local ip,parse the args, create socket
     *  and accept connections
     *  If there is an error in any step, it will exit and send a signal to
     *  the process itself
     */

     ServerParams * params=(ServerParams*)args;
    int port = LISTENPORT;//default port
    if(get_local_ip(local_ip) || parse_arg(params->argc, params->argv, &port, rootDir)){
        return NULL;
    }


    int listenfd, connfd;
    struct sockaddr_in addr;
    pthread_t threads[MAX_CONNECTION];
    User users[MAX_CONNECTION];
    for (int i = 0; i < MAX_CONNECTION; i++) {
        users[i].userNo = -1;
    }

    //create socket
    if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        return NULL;
    }

    //set local ip and port
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listenfd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        return NULL;
    }


    if (listen(listenfd, 10) == -1) {
        return NULL;
    }


    while (running) {

        if ((connfd = accept(listenfd, NULL, NULL)) == -1) {
            continue;
        }
        int p;
        if (num_threads < MAX_CONNECTION) {
            for (p = 0; p < MAX_CONNECTION; p++) {
                if (users[p].userNo == -1) {
                    users[p].connfd = connfd;
                    users[p].state = NOTLOGIN;
                    users[p].userNo = p;
                    num_threads++;
                    strcpy(users[p].dir, "/");
                    if (pthread_create(&threads[p], NULL, main_process, &users[p])) {
                        send_message(connfd,SERVICE_UNAVAILABLE);
                    }
                    break;
                }
            }
        } else {
            send_message(connfd,MAX_CONNECTIONS_REACHED);
            close(connfd);
        }
    }
    //Send 421 to all clients
    for(int i=0;i<MAX_CONNECTION;i++){
        if(users[i].userNo<0)continue;
        close(users[i].filefd);
        fclose(users[i].fp);
        send_message(users[i].connfd,SERVICE_UNAVAILABLE);
        close(users[i].connfd);
    }
    close(listenfd);
    return NULL;
}

void *main_process(void *args) {
    User *user = (User *) args;
    if (send_message(user->connfd, GREETING)) {
        user->userNo = -1;
        num_threads--;
        return NULL;
    }
    char sentence[8192];
    user->state = NOTLOGIN;
    receive_message(user->connfd, sentence);
    while (handle_command(user, sentence) == 0) {
        receive_message(user->connfd, sentence);
    }
    close(user->connfd);
    num_threads--;
    user->userNo = -1;
    return NULL;
}

int handle_command(User *user, char *sentence) {
    char command[5];
    memcpy(command, sentence, 4 * sizeof(char));
    command[4] = '\0';
    if (strcmp(command, "LIST") == 0) {
        return handle_LIST(user, sentence);
    }
    if (strcmp(command, "PWD\r") == 0) {
        return handle_PWD(user, sentence);
    }
    if (strcmp(command, "PORT") == 0) {
        return handle_PORT(user, sentence);
    }
    if (strcmp(command, "PASV") == 0) {
        return handle_PASV(user, sentence);
    }
    if (strcmp(command, "RETR") == 0) {
        return handle_RETR(user, sentence);
    }
    if (strcmp(command, "TYPE") == 0) {
        return handle_TYPE(user, sentence);
    }
    if (strcmp(command, "STOR") == 0) {
        return handle_STOR(user, sentence);
    }
    if (strcmp(command, "QUIT") == 0 || strcmp(command, "ABOR") == 0) {
        return handle_QUIT(user, sentence);
    }
    if (strcmp(command, "MKD ") == 0) {
        return handle_MKD(user, sentence);
    }
    if (strcmp(command, "CWD ") == 0) {
        return handle_CWD(user, sentence);
    }
    if (strcmp(command, "RMD ") == 0) {
        return handle_RMD(user, sentence);
    }
    if (strcmp(command, "RNTO") == 0) {
        return handle_RNTO(user, sentence);
    }
    if (strcmp(command, "RNFR") == 0) {
        return handle_RNFR(user, sentence);
    }
    if (strcmp(command, "USER") == 0) {
        return handle_USER(user, sentence);
    }
    if (strcmp(command, "PASS") == 0) {
        return handle_PASS(user, sentence);
    }
    if (strcmp(command, "SYST") == 0) {
        return handle_SYST(user, sentence);
    }
    send_message(user->connfd, SERVICE_UNAVAILABLE);
    return -1;
}