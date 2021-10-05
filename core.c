//
// Created by 邱俣涵 on 2021/10/4.
//

#include "core.h"

int running=1;
int init_server(){
    int listenfd, connfd;		//监听socket和连接socket不一样，后者用于数据传输
    struct sockaddr_in addr;
    pthread_t threads[10];
    int num_threads=0;
    User users[10];
    int p=0;
    printf("test output\n");
    //创建socket
    if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("Error socket(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }

    //设置本机的ip和port
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(LISTENPORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);	//监听"0.0.0.0"

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

    //持续监听连接请求
    while (running) {
        //等待client的连接 -- 阻塞函数
        if ((connfd = accept(listenfd, NULL, NULL)) == -1) {
            printf("Error accept(): %s(%d)\n", strerror(errno), errno);
            continue;
        }
        users[p].connfd=connfd;
        users[p].state=NOTLOGIN;
        //TODO:应对用户过多，创建进程失败等情况
        pthread_create(&threads[num_threads++],NULL,main_process,&users[p]);
        p++;
    }
    close(listenfd);
}

void* main_process(void* args){
    User* user=(User*)args;
    if(init_connection(user->connfd)!=0){
        //TODO:释放用户并告知主要线程
        return NULL;
    }
    char sentence[8192];
    int len;
    user->state=LOGIN;
    receive_message(user->connfd,sentence,&len);
    printf("%s\n",sentence);
    while(handle_command(user,sentence)==0) {
        receive_message(user->connfd, sentence, &len);
    }
    close(user->connfd);

}
int init_connection(int connfd){
    char sentence[8192];
    int p;
    int len;
    //持续监听连接请求
    while (1) {
        //发送字符串到socket,问候信息
        char greet_sentence[]="220 Anonymous FTP server ready.\r\n";
        int len= strlen(greet_sentence);
        send_message(connfd,greet_sentence,len);
        printf("wait client\n");
        receive_message(connfd,sentence,&len);
        printf("%s\n",sentence);
        printf("%d\n",strcmp(sentence,"USER anonymous\r\n") );
        if(strcmp(sentence,"USER anonymous\r\n")==0){
            char verify_sentence[]="331 Guest login ok\r\n";
            send_message(connfd,verify_sentence,strlen(verify_sentence));
        }else{
            char refuse_sentence[]="530 wrong user name.\r\n";
            send_message(connfd,refuse_sentence, strlen(refuse_sentence));
        }
        printf("wait client\n");
        receive_message(connfd,sentence,&len);
        printf("%s %d\n",sentence,len);
        char log_in_message[]="230 Guest login ok.\r\n";
        return send_message(connfd,log_in_message, strlen(log_in_message));
    }
}
int handle_command(User *user, char* sentence){
    char command[5];
    memcpy(command,sentence,4*sizeof(char));
    command[4]='\0';
    if(strcmp(command,"SYST")==0){
        return handle_syst(user, sentence);
    }
    if(strcmp(command,"PORT")==0){
        return handle_port(user,sentence);
    }
    if(strcmp(command,"PASV")==0){
        return handle_pasv(user,sentence);
    }

    return -1;

}