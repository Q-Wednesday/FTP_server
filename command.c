//
// Created by 邱俣涵 on 2021/10/4.
//

//return 1表示正常进行，return 0 表示错误
#include "command.h"

#include <stdlib.h>

#define  SYS_INFO "215 UNIX Type: L8\r\n"
#define ACCEPT_PORT "200 PORT command successful\r\n"
#define ENTER_PASV "227 Entering Passive Mode (%s,%d,%d)\r\n"
int handle_syst(User *user, char*sentence){
    if(strcmp(sentence,"SYST\r\n")==0){
        return send_message(user->connfd, SYS_INFO, strlen(SYS_INFO));
    }
    return -1;
}
int parse_ip(User* user,char* sentence){
    char ip[20];
    int count=0,p=0,q=0;//p->sentence,q->ip
    user->addr.sin_family=AF_INET;
    while (count<4 && sentence[p]!='\0'){
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
    while(sentence[p]!='\0' && sentence[p]!=','){
        if(sentence[p]>'0' && sentence[p]<='9'){
            port_num=port_num*10+(sentence[p]-'0');
            p++;
        }else{
            return -2;//port解析错误
        }
    }
    int p2=0;
    while(sentence[p]!='\0'){
        if(sentence[p]>'0' && sentence[p]<='9'){
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
        return send_message(user->connfd,ACCEPT_PORT, strlen(ACCEPT_PORT));
    }
    return -1;

}


int handle_pasv(User *user, char *sentence) {

    struct sockaddr_in addr;
    //设置本机的ip和port
    memset(&addr, 0, sizeof(addr));
    int random_port=rand()%45535+20000;

    char message[60];
    sprintf(message,ENTER_PASV,"127,0,0,1",random_port/256,random_port%256);
    send_message(user->connfd,message, strlen(message));
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

    if ((user->datafd = accept(listenfd, NULL, NULL)) == -1) {
        printf("Error accept(): %s(%d)\n", strerror(errno), errno);
        return -1;
    }

//    getsockname(user->connfd,(struct sockaddr*)&myaddr,NULL);
//    char* ip= inet_ntoa(myaddr.sin_addr);
//    int port=ntohs(myaddr.sin_port);

    return 0;
}
