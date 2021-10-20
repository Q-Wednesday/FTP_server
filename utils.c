//
// Created by 邱俣涵 on 2021/10/20.
//
#include "utils.h"
#include "IO.h"
#define REJECT_FILE_TRANS_NOT_CON "425 No TCP connection was established\r\n"
int parse_ip(User *user, const char *sentence) {
    char ip[20];
    int count = 0, p = 0, q = 0;//p->sentence,q->ip
    user->addr.sin_family = AF_INET;
    while (count < 4 && sentence[p] != '\r') {
        if (sentence[p] >= '0' && sentence[p] <= '9') {
            ip[q] = sentence[p];
            p++;
            q++;
        } else if (sentence[p] == ',') {
            ip[q] = '.';
            p++;
            q++;
            count++;
        } else {
            return -1;//ip解析错误
        }
    }
    ip[q - 1] = '\0';//结尾置为结束
    user->addr.sin_addr.s_addr = inet_addr(ip);
    int port_num = 0;
    while (sentence[p] != '\r' && sentence[p] != ',') {
        if (sentence[p] >= '0' && sentence[p] <= '9') {
            port_num = port_num * 10 + (sentence[p] - '0');
            p++;
        } else {
            return -2;//port解析错误
        }
    }
    p++;//p指向p2开头
    int p2 = 0;
    while (sentence[p] != '\r') {
        if (sentence[p] >= '0' && sentence[p] <= '9') {
            p2 = p2 * 10 + sentence[p] - '0';
            p++;
        } else {
            return -2;//port解析错误
        }
    }
    user->addr.sin_port = htons(port_num * 256 + p2);
    return 0;
}
int connect_filefd(User *user, char *sentence) {
    //只用于进行连接，连接后的消息应该由调用者发送
    if (user->state != PORTMODE && user->state != PASVMODE) {
        send_message(user->connfd, REJECT_FILE_TRANS_NOT_CON);
        return -1;
    }
    if (user->state == PORTMODE) {
        //连接上目标主机（将socket和目标主机连接）-- 阻塞函数
        if (connect(user->filefd, (struct sockaddr *) &user->addr, sizeof(user->addr)) < 0) {
            printf("Error connect(): %s(%d)\n", strerror(errno), errno);
            return 1;
        }
        return 0;
    } else {
        return 0;
    }
}
int parse_dir(char *user_path_parsed, char *source, User *user) {
    //source是收到的文件夹名称
    char user_path[MAX_MESSAGE_SIZE * 2];
    //进行对路径的处理，主要做法是忽略./和回退../。如果回退到了根目录就不再回退
    size_t len = strlen(source);
    source[len - 2] = '\0';
    if (source[0] == '/') {
        //绝对路径
        sprintf(user_path, "%s", source);
    } else {
        if (strlen(user->dir) == 1)
            sprintf(user_path, "%s%s", user->dir, source);
        else
            sprintf(user_path, "%s/%s", user->dir, source);
    }
    int p = 0, q = 0;//p指向原本的，q指向目标
    while (user_path[p] != '\0') {
        if (user_path[p] != '.') {
            user_path_parsed[q] = user_path[p];
            p++;
            q++;
            continue;
        }
        if (user_path[p] == '.') {
            // ./的情况
            if (user_path[p - 1] == '/' && (user_path[p + 1] == '/' || user_path[p + 1] == '\0')) {
                p += 2;
                continue;
            }
            // ..的情况
            if (user_path[p - 1] == '/' && user_path[p + 1] == '.') {
                if (user_path[p + 2] == '/')p++;
                p += 2;
                q -= 2;
                while (user_path_parsed[q] != '/' && q >= 0) {
                    q--;
                }
                if (q < 0) {
                    //根目录了
                    return 1;
                }
                q++;
            } else {
                user_path_parsed[q] = user_path[p];
                p++;
                q++;
            }
        }
    }
    if (user_path_parsed[q - 1] == '/' && q != 1) {
        user_path_parsed[q - 1] = '\0';//把最后的/去除
    } else
        user_path_parsed[q] = '\0';
    return 0;
}
