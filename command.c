//
// Created by 邱俣涵 on 2021/10/4.
//

//return 1表示正常进行，return 0 表示错误
#include "command.h"

#include <stdlib.h>

#define  SYS_INFO "215 UNIX Type: L8\r\n"
#define ACCEPT_PORT "200 PORT command successful\r\n"
#define ENTER_PASV "227 Entering Passive Mode (%s,%d,%d)\r\n"
#define ACCEPT_CONNECTION "150 Opening BINARY mode data connection for %s \r\n"
#define REJECT_FILE_TRANS_NOT_CON "425 No TCP connection was established\r\n"
#define ACCECPT_FILE_TRANS "226 Transfer complete\r\n"
#define REJECT_RETR_READFILE "451 The server had trouble reading the file\r\n"
#define  ACCEPT_TYPE "200 Type set to I\r\n"

extern char dir[100];
int handle_syst(User *user, char*sentence){
    if(strcmp(sentence,"SYST\r\n")==0){
        return send_message(user->connfd, SYS_INFO);
    }
    return -1;
}
int parse_ip(User* user,char* sentence){
    char ip[20];
    int count=0,p=0,q=0;//p->sentence,q->ip
    user->addr.sin_family=AF_INET;
    while (count<4 && sentence[p]!='\r'){
        if(sentence[p]>='0' && sentence[p]<='9'){
            ip[q]= sentence[p];
            p++;q++;
        }else if(sentence[p]==','){
            ip[q]='.';
            p++;q++;count++;
        }else{
            return -1;//ip解析错误
        }
    }
    ip[q-1]='\0';//结尾置为结束
    user->addr.sin_addr.s_addr=inet_addr(ip);
    //TODO:解析port
    int port_num=0;
    while(sentence[p]!='\r' && sentence[p]!=','){
        if(sentence[p]>='0' && sentence[p]<='9'){
            port_num=port_num*10+(sentence[p]-'0');
            p++;
        }else{
            return -2;//port解析错误
        }
    }
    p++;//p指向p2开头
    int p2=0;
    while(sentence[p]!='\r'){
        if(sentence[p]>='0' && sentence[p]<='9'){
            p2=p2*10+sentence[p]-'0';
            p++;
        }else{
            return -2;//port解析错误
        }
    }
    user->addr.sin_port= htons(port_num*256+p2);
    return 0;

}
int handle_port(User *user, char*sentence){

    int p=0;
    while(sentence[p]<'0' || sentence[p]>'9'){
        p++;
    }
    if(parse_ip(user,sentence+p)==0){
        user->state=PORTMODE;
        printf("set to portmode\n");
        return send_message(user->connfd, ACCEPT_PORT);
    }
    return -1;

}
int handle_pasv(User *user, char *sentence) {

    struct sockaddr_in addr;
    //设置本机的ip和port
    memset(&addr, 0, sizeof(addr));
    int random_port=rand()%45535+20000;

    char message[60];

    addr.sin_family = AF_INET;
    addr.sin_port = htons(random_port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);	//监听"0.0.0.0"
    int listenfd;
    //创建socket
    if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("Error socket(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }
    //将本机的ip和port与socket绑定
    if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        printf("Error bind(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }
    //开始监听socket
    if (listen(listenfd, 10) == -1) {
        printf("Error listen(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }
    sprintf(message,ENTER_PASV,"127,0,0,1",random_port/256,random_port%256);
    send_message(user->connfd, message);
    if ((user->datafd = accept(listenfd, NULL, NULL)) == -1) {
        printf("Error accept(): %s(%d)\n", strerror(errno), errno);
        return -1;
    }
    user->state=PASVMODE;
//    getsockname(user->connfd,(struct sockaddr*)&myaddr,NULL);
//    char* ip= inet_ntoa(myaddr.sin_addr);
//    int port=ntohs(myaddr.sin_port);

    return 0;
}
int handle_retr(User* user,char* sentence) {
    //TODO:解析文件名目前就按照第五个字符开始是文件名处理.进行异常处理
    //重构该函数与stor，二者共性多
    char filename[100],message[100];
    printf("dir:%s\n",dir);
    sprintf(filename,"%s/%s", dir, sentence + 5);
    int len= strlen(filename);
    filename[len-2]='\0';//去掉\r\n
    if (user->state == PORTMODE) {
        int sockfd;
        if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
            printf("Error socket(): %s(%d)\n", strerror(errno), errno);
            return 1;
        }
        //连接上目标主机（将socket和目标主机连接）-- 阻塞函数
        if (connect(sockfd, (struct sockaddr *) &user->addr, sizeof(user->addr)) < 0) {
            printf("Error connect(): %s(%d)\n", strerror(errno), errno);
            return 1;
        }
        user->datafd = sockfd;

        sprintf(message,ACCEPT_CONNECTION, sentence + 5);
        send_message(user->connfd, message);

    } else if (user->state == PASVMODE) {
        sprintf(message,ACCEPT_CONNECTION, sentence + 5);
        send_message(user->connfd, message);
    }
    else{
        send_message(user->connfd, REJECT_FILE_TRANS_NOT_CON);
        return -1;
    }
    int result=send_file(user->datafd,filename);

    user->state=LOGIN;
    if(result==0){

        close(user->datafd);
        //setsockopt(user->connfd, IPPROTO_TCP, TCP_NODELAY)
        send_message(user->connfd, ACCECPT_FILE_TRANS);

    } else if(result==-1){
        send_message(user->connfd, REJECT_RETR_READFILE);
        close(user->datafd);
    }
    return result;
}
int handle_type(User* user,char* sentence){
    //TODO: 暂时只匹配第五个字符
    if(sentence[5]=='I'){
        return send_message(user->connfd,ACCEPT_TYPE);
    }
}

int handle_stor(User* user,char* sentence){
    char filename[100],message[100];
    printf("dir:%s\n",dir);
    sprintf(filename,"%s/%s", dir, sentence + 5);
    int len= strlen(filename);
    filename[len-2]='\0';//去掉\r\n
    if (user->state == PORTMODE) {
        int sockfd;
        //创建过程应该留到port
        if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
            printf("Error socket(): %s(%d)\n", strerror(errno), errno);
            return 1;
        }
        //连接上目标主机（将socket和目标主机连接）-- 阻塞函数
        if (connect(sockfd, (struct sockaddr *) &user->addr, sizeof(user->addr)) < 0) {
            printf("Error connect(): %s(%d)\n", strerror(errno), errno);
            return 1;
        }
        user->datafd = sockfd;

        sprintf(message,ACCEPT_CONNECTION, sentence + 5);
        send_message(user->connfd, message);

    } else if (user->state == PASVMODE) {
        sprintf(message,ACCEPT_CONNECTION, sentence + 5);
        send_message(user->connfd, message);
    }
    else{
        send_message(user->connfd, REJECT_FILE_TRANS_NOT_CON);
        return -1;
    }
    int result= receive_file(user->datafd,filename);
    user->state=LOGIN;
    //close(user->datafd);
    //user->datafd=-1;
    if(result==0){
        return send_message(user->connfd, ACCECPT_FILE_TRANS);
    } else if(result==-1){
        return send_message(user->connfd, REJECT_RETR_READFILE);
    }
}