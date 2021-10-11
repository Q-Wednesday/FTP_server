//
// Created by 邱俣涵 on 2021/10/4.
//

#include "core.h"
char dir[MAX_MESSAGE_SIZE]="/tmp";
int running=1;
int parse_arg(int argc,char** argv,char* root,int* port){
    for(int i=1;i<argc;i++){
        if(strcmp(argv[i],"-port")==0){
            *port=0;
            int j=0;
            while(argv[i+1][j]!='\0'){
                if(argv[i+1][j]>'9'||argv[i+1][j]<'0') {
                    printf("cannot parse port number\n");
                    return -1;
                }
                *port=*port*10+argv[i+1][j]-'0';
                j++;
            }
            i++;
        }else if(strcmp(argv[i],"-root")==0){
            strcpy(dir,argv[i+1]);
            i++;
        }else{
            printf("cannot parse the args\n");
            return -2;
        }
    }
    return 0;
}
int init_server(int argc, char **argv) {
    /**
     * return -1 解析参数失败
     */
    int port=LISTENPORT;

    if(parse_arg(argc,argv,dir,&port)!=0){
        return  -1;
    }
    printf("port: %d,root:%s\n",port,dir);
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
    addr.sin_port = htons(port);
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
        //TODO: 应对前面有用户已经释放的情况
        users[p].connfd=connfd;
        users[p].state=NOTLOGIN;
        strcpy(users[p].dir,"/");//默认进入的是根文件夹
        //TODO:应对用户过多，创建进程失败等情况
        pthread_create(&threads[num_threads++],NULL,main_process,&users[p]);
        printf("create user\n");
        p++;
    }
    close(listenfd);
    return 0;
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
    return NULL;
}
int init_connection(int connfd){
    char sentence[8192];
    //int p;
    //int len;
    //持续监听连接请求
    while (1) {
        //发送字符串到socket,问候信息
        char greet_sentence[]="220 Anonymous FTP server ready.\r\n";
        int len= strlen(greet_sentence);
        send_message(connfd, greet_sentence);
        printf("wait client\n");
        receive_message(connfd,sentence,&len);
        printf("%s\n",sentence);
        printf("%d\n",strcmp(sentence,"USER anonymous\r\n") );
        if(strcmp(sentence,"USER anonymous\r\n")==0){
            char verify_sentence[]="331 Guest login ok\r\n";
            send_message(connfd, verify_sentence);
        }else{
            char refuse_sentence[]="530 wrong user name.\r\n";
            send_message(connfd, refuse_sentence);
        }
        printf("wait client\n");
        receive_message(connfd,sentence,&len);
        printf("%s %d\n",sentence,len);
        char log_in_message[]="230 Guest login ok.\r\n";
        return send_message(connfd, log_in_message);
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
    if(strcmp(command,"RETR")==0){
        return handle_retr(user,sentence);
    }
    if(strcmp(command,"TYPE")==0){
        return handle_type(user,sentence);
    }
    if(strcmp(command,"STOR")==0){
        return handle_stor(user,sentence);
    }
    if(strcmp(command,"QUIT")==0 || strcmp(command,"ABOR")==0){
        return handle_quit(user,sentence);
    }
    if(strcmp(command,"LIST")==0){
        return handle_list(user,sentence);
    }
    if(strcmp(command,"PWD\r")==0){
        return handle_pwd(user,sentence);
    }
    if(strcmp(command,"MKD ")==0){
        return handle_mkd(user,sentence);
    }
    if(strcmp(command,"CWD ")==0){
        return handle_cwd(user,sentence);
    }
    if(strcmp(command,"RMD ")==0){
        return handle_rmd(user,sentence);
    }
    if(strcmp(command,"RNTO")==0){
        return handle_rnto(user,sentence);
    }
    if(strcmp(command,"RNFR")==0){
        return handle_rnfr(user,sentence);
    }


    return -1;

}