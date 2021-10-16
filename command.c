//
// Created by 邱俣涵 on 2021/10/4.
//

//return 1表示正常进行，return 0 表示错误
#include "command.h"

#include <stdlib.h>
#include <fcntl.h>

#define  SYS_INFO "215 UNIX Type: L8\r\n"
#define ACCEPT_PORT "200 PORT command successful\r\n"
#define ENTER_PASV "227 Entering Passive Mode (%s,%d,%d)\r\n"
#define ACCEPT_CONNECTION "150 Opening BINARY mode data connection for %s \r\n"
#define ACCEPT_CONNECTION_ASCII "150 Opening ASCII mode data connection for %s \r\n"
#define REJECT_FILE_TRANS_NOT_CON "425 No TCP connection was established\r\n"
#define ACCECPT_FILE_TRANS "226 Transfer complete\r\n"
#define REJECT_RETR_READFILE "451 The server had trouble reading the file\r\n"
#define  ACCEPT_TYPE "200 Type set to I.\r\n"
#define GOODBYE "221 Good Bye\r\n"
#define LS_COMMAND "ls -l %s%s"
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
extern char dir[MAX_MESSAGE_SIZE];
extern char local_ip[20];
int handle_syst(User *user, char *sentence) {
    if (strcmp(sentence, "SYST\r\n") == 0) {
        return send_message(user->connfd, SYS_INFO);
    }
    return -1;
}

int parse_ip(User *user, char *sentence) {
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
    //TODO:解析port
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

int handle_port(User *user, char *sentence) {

    int p = 0;
    while (sentence[p] < '0' || sentence[p] > '9') {
        p++;
    }
    if (parse_ip(user, sentence + p) != 0) {
        return -1;
    }


    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("Error socket(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }
    user->filefd = sockfd;
    user->state=PORTMODE;
    //printf("set to portmode\n");
    return send_message(user->connfd, ACCEPT_PORT);


}

int handle_pasv(User *user, char *sentence) {
    if(user->state==PASVMODE){
        //丢弃已有连接
        close(user->filefd);
        user->state=LOGIN;
    }
    struct sockaddr_in addr;
    //设置本机的ip和port
    memset(&addr, 0, sizeof(addr));
    int random_port = rand() % 45535 + 20000;

    char message[60];

    addr.sin_family = AF_INET;
    addr.sin_port = htons(random_port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);    //监听"0.0.0.0"
    int listenfd;
    //创建socket
    if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("Error socket(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }
    //将本机的ip和port与socket绑定
    if (bind(listenfd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        printf("Error bind(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }
    //开始监听socket
    if (listen(listenfd, 10) == -1) {
        printf("Error listen(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }

    sprintf(message, ENTER_PASV, local_ip, random_port / 256, random_port % 256);
    send_message(user->connfd, message);
    if ((user->filefd = accept(listenfd, NULL, NULL)) == -1) {
        printf("Error accept(): %s(%d)\n", strerror(errno), errno);
        return -1;
    }
    user->state = PASVMODE;
//    getsockname(user->connfd,(struct sockaddr*)&myaddr,NULL);
//    char* ip= inet_ntoa(myaddr.sin_addr);
//    int port=ntohs(myaddr.sin_port);

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
        //设置不阻塞
        fcntl(user->filefd, F_SETFL, O_NONBLOCK);
        return 0;
    }else{
        return 0;
    }
}

int handle_retr(User *user, char *sentence) {
    //TODO:解析文件名目前就按照第五个字符开始是文件名处理.进行异常处理,return 非0的地方
    //重构该函数与stor，二者共性多
    char filename[MAX_DATA_SIZE],message[MAX_MESSAGE_SIZE];
    printf("dir:%s\n", dir);
    int len = strlen(sentence);
    sentence[len - 2] = '\0';//去掉\r\n
    sprintf(filename, "%s%s/%s", dir,user->dir, sentence + 5);
    if(connect_filefd(user, sentence)!=0){
        return -1;
    }
    sprintf(message, ACCEPT_CONNECTION, sentence + 5);
    send_message(user->connfd, message);
    user->fp = fopen(filename, "rb");
    if (!user->fp) {
        send_message(user->connfd, REJECT_RETR_READFILE);
        close(user->filefd);
    }
    int result = pthread_create(&user->file_thread, NULL, &send_file, user);

    return result;
}

int handle_type(User *user, char *sentence) {
    //TODO: 只匹配第五个字符。且A为测试用，因为ftp总会在ls的时候发。
    if (sentence[5] == 'I' ||sentence[5]=='A') {
        return send_message(user->connfd, ACCEPT_TYPE);
    }
    return -1;
}

int handle_stor(User *user, char *sentence) {
    char filename[MAX_DATA_SIZE],message[MAX_MESSAGE_SIZE];
    printf("dir:%s\n", dir);
    int len = strlen(sentence);
    sentence[len - 2] = '\0';//去掉\r\n
    sprintf(filename, "%s%s/%s", dir, user->dir,sentence + 5);
    if(connect_filefd(user,sentence)!=0){
        return -1;
    }
    sprintf(message, ACCEPT_CONNECTION, sentence + 5);
    send_message(user->connfd, message);
    int result = receive_file(user->filefd, filename);
    user->state = LOGIN;
    //close(user->filefd);
    //user->filefd=-1;
    if (result == 0) {
        return send_message(user->connfd, ACCECPT_FILE_TRANS);
    } else if (result == -1) {
        return send_message(user->connfd, REJECT_RETR_READFILE);
    }
    return -2;
}

int handle_quit(User *user, char *sentence) {
    //TODO:终止用户正在运行的东西
    send_message(user->connfd, GOODBYE);//关闭是在主过程中
    return -1;//固定返回-1表示关闭

}

int parse_dir(char* user_path_parsed,char*source,User* user){
    //source是收到的文件夹名称
    char user_path[MAX_MESSAGE_SIZE];
    //进行对路径的处理，主要做法是忽略./和回退../。如果回退到了根目录就不再回退
    size_t len= strlen(source);
    source[len-2]='\0';
//    if(source[len-3]=='/') {
//        source[len - 2] = '\0';
//    }
//    else {
//        source[len - 2] = '/';
//        source[len - 1] = '\0';
//    }
    if(source[0]=='/'){
        //绝对路径
        sprintf(user_path,"%s",source);
    }else{
        if(strlen(user->dir)==1)
            sprintf(user_path,"%s%s",user->dir,source);
        else
            sprintf(user_path,"%s/%s",user->dir,source);
    }
    int p=0,q=0;//p指向原本的，q指向目标
    while (user_path[p]!='\0'){
        if(user_path[p]!='.'){
            user_path_parsed[q]=user_path[p];
            p++;q++;continue;
        }
        if(user_path[p]=='.'){
            // ./的情况
            if(user_path[p-1]=='/' && (user_path[p+1]=='/'|| user_path[p+1]=='\0')){
                p+=2;continue;
            }
            // ..的情况
            if(user_path[p-1]=='/' && user_path[p+1]=='.'){
                if(user_path[p+2]=='/')p++;
                p+=2;
                q-=2;
                while (user_path_parsed[q]!='/'&& q>=0){
                    q--;
                }
                if(q<0){
                    //根目录了
                    return 1;
                }
                q++;
            }
            else{
                user_path_parsed[q]=user_path[p];
                p++;q++;
            }
        }
    }
    if(user_path_parsed[q-1]=='/'&& q!=1){
        user_path_parsed[q-1]='\0';//把最后的/去除
    }else
        user_path_parsed[q]='\0';
    return 0;
}
int handle_list(User* user,char* sentence){
    //使用管道执行ls命令并获取输出
    //TODO:没做错误的处理
    char command[MAX_DATA_SIZE],message[MAX_MESSAGE_SIZE];
    if(strlen(sentence)<=6){
        sprintf(command,LS_COMMAND,dir,user->dir);
    }else{
        char user_path_parsed[MAX_MESSAGE_SIZE];
        if(parse_dir(user_path_parsed,sentence+5,user)){
            return send_message(user->connfd,CWD_FAILED);
        }
        sprintf(command,LS_COMMAND,dir,user_path_parsed);
    }
    if(connect_filefd(user,sentence)!=0){
        return -1;
    }
    sprintf(message,ACCEPT_CONNECTION_ASCII,"ls");
    send_message(user->connfd,message);
    user->fp= popen(command,"r");
    int result= pthread_create(&user->file_thread,NULL,&send_file,user);
    return result;

}
int handle_mkd(User* user,char* sentence){
    //TODO:处理创建出错的情况.不允许出现../
    char command[MAX_DATA_SIZE],user_path_parsed[MAX_MESSAGE_SIZE];//message[MAX_MESSAGE_SIZE],
    if(parse_dir(user_path_parsed,sentence+4,user)){
        return send_message(user->connfd,MKD_FAILED);
    }

    sprintf(command,MKD_COMMAND,dir,user_path_parsed);
    //sprintf(message,ACCEPT_DIR,full_path);
//    if(send_message(user->connfd,message)){
//        return -1;
//    }
    if(system(command)==0){
        return send_message(user->connfd,MKD_SUCCESS);
    }

    return send_message(user->connfd,MKD_FAILED);
}

int handle_pwd(User*user,char*sentence){
    char message[MAX_DATA_SIZE];
    sprintf(message, ACCEPT_DIR, user->dir);
    return send_message(user->connfd,message);
}
int handle_cwd(User*user,char* sentence){

    char command[MAX_DATA_SIZE],full_path[MAX_MESSAGE_SIZE*2],
    user_path_parsed[MAX_MESSAGE_SIZE];

    if(parse_dir(user_path_parsed,sentence+4,user)){
        return send_message(user->connfd,CWD_FAILED);
    }

    sprintf(full_path,"%s%s",dir,user_path_parsed);
    sprintf(command,CWD_COMMAND,full_path);
    if(system(command)==0){
        strcpy(user->dir,user_path_parsed);
        return send_message(user->connfd,CWD_SUCCESS);
    }
    return send_message(user->connfd,CWD_FAILED);
}

int handle_rmd(User*user,char* sentence){
    //TODO:处理权限

    char command[MAX_DATA_SIZE],user_path_parsed[MAX_MESSAGE_SIZE];
    if(parse_dir(user_path_parsed,sentence+4,user)){
        return send_message(user->connfd,RMD_FAILED);
    }
    sprintf(command,RMD_COMMAND,dir,user_path_parsed);
    if(system(command)==0){
        return send_message(user->connfd,RMD_SUCCESS);
    }
    return send_message(user->connfd,RMD_FAILED);
}

int handle_rnfr(User*user,char*sentence){
    //先检测文件是否存在
    char user_path_parsed[MAX_MESSAGE_SIZE*2],command[MAX_DATA_SIZE];
    if(parse_dir(user_path_parsed,sentence+5,user)){
        return send_message(user->connfd,REJECT_RNFR_A);
    }
    sprintf(user->filename,"%s%s",dir,user_path_parsed);
    sprintf(command,"ls %s",user->filename);
    if(system(command)){
        return send_message(user->connfd,REJECT_RNFR_NO_FILE);
    }
    return send_message(user->connfd,ACCEPT_RNFR);
}
int handle_rnto(User*user, char*sentence){
    char user_path_parsed[MAX_MESSAGE_SIZE*2],command[MAX_DATA_SIZE];
    if(parse_dir(user_path_parsed,sentence+5,user)){
        return send_message(user->connfd,REJECT_RNTO);
    }
    sprintf(command,RNTO_COMMAND,user->filename,dir,user_path_parsed);
    if(system(command)){
        return send_message(user->connfd,REJECT_RNTO);
    }
    return send_message(user->connfd,ACCEPT_RNTO);
}