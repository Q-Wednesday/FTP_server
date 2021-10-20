//
// Created by 邱俣涵 on 2021/10/4.
//
#include "core.h"

#define GREETING "220 Anonymous FTP server ready.\r\n"
#define SERVICE_UNAVAILABLE "421 Service not available, remote server has closed connection\r\n"
char rootDir[MAX_MESSAGE_SIZE] = "/tmp";
char local_ip[20];
int running = 1;
int num_threads = 0;//Number of running threads
void get_local_ip(char *buffer) {
    const char *google_dns_server = "8.8.8.8";
    int dns_port = 53;
    struct sockaddr_in serv;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    //Socket could not be created
    if (sock < 0) {
        perror("Socket error");
    }

    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr(google_dns_server);
    serv.sin_port = htons(dns_port);
    //TODO:错误处理
    if (connect(sock, (const struct sockaddr *) &serv, sizeof(serv))) {

    }

    struct sockaddr_in name;
    socklen_t namelen = sizeof(name);
    getsockname(sock, (struct sockaddr *) &name, &namelen);
    const char *p = inet_ntop(AF_INET, &name.sin_addr, buffer, 100);
    if (!p) {
        //Some error
        printf("Error number : %d . Error message : %s \n", errno, strerror(errno));
    }
    for (int i = 0; i < strlen(buffer); i++) {
        //转化为逗号分隔
        if (buffer[i] == '.') {
            buffer[i] = ',';
        }
    }
    close(sock);
}

int parse_arg(int argc, char **argv, int *port) {
    //parse the arguments of the program
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-port") == 0) {
            *port = 0;
            int j = 0;
            while (argv[i + 1][j] != '\0') {
                if (argv[i + 1][j] > '9' || argv[i + 1][j] < '0') {
                    printf("cannot parse port number\n");
                    return -1;
                }
                *port = *port * 10 + argv[i + 1][j] - '0';
                j++;
            }
            i++;
        } else if (strcmp(argv[i], "-root") == 0) {
            strcpy(rootDir, argv[i + 1]);
            i++;
        } else {
            printf("cannot parse the args\n");
            return -2;
        }
    }
    return 0;
}

void* init_server(void* args) {
    /**
     *
     *
     */
     ServerParams * params=(ServerParams*)args;
    int port = LISTENPORT;//default port
    get_local_ip(local_ip);
    if (parse_arg(params->argc, params->argv, &port) != 0) {
        return NULL;
    }
    //printf("port: %d,root:%s\n",port,rootDir);
    int listenfd, connfd;
    struct sockaddr_in addr;
    pthread_t threads[MAX_CONNECTION];
    User users[MAX_CONNECTION];
    for (int i = 0; i < MAX_CONNECTION; i++) {
        users[i].userNo = -1;
    }

    //创建socket
    if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("Error socket(): %s(%d)\n", strerror(errno), errno);
        return NULL;
    }

    //设置本机的ip和port
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);    //监听"0.0.0.0"

    //将本机的ip和port与socket绑定
    if (bind(listenfd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        printf("Error bind(): %s(%d)\n", strerror(errno), errno);
        return NULL;
    }

    //开始监听socket
    if (listen(listenfd, 10) == -1) {
        printf("Error listen(): %s(%d)\n", strerror(errno), errno);
        return NULL;
    }
    //持续监听连接请求

    while (running) {
        //等待client的连接 -- 阻塞函数
        if ((connfd = accept(listenfd, NULL, NULL)) == -1) {
            printf("Error accept(): %s(%d)\n", strerror(errno), errno);
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
                    strcpy(users[p].dir, "/");//默认进入的是根文件夹
                    if (pthread_create(&threads[p], NULL, main_process, &users[p])) {
                        printf("Error pthread_create(): %s(%d)\n", strerror(errno), errno);
                    }
                    break;
                }
            }
        } else {
            printf("The maximum number of connections has been reached\n");
            close(connfd);
        }
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
    int len;
    user->state = NOTLOGIN;
    receive_message(user->connfd, sentence, &len);
    printf("%s\n", sentence);
    while (handle_command(user, sentence) == 0) {
        receive_message(user->connfd, sentence, &len);
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