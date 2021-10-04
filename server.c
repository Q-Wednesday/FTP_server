#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include "server.h"
#define LISTENPORT 21

int main(int argc, char **argv) {
	int listenfd, connfd;		//监听socket和连接socket不一样，后者用于数据传输
	struct sockaddr_in addr;
	char sentence[8192];
	int p;
	int len;
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
	while (1) {
		//等待client的连接 -- 阻塞函数
		if ((connfd = accept(listenfd, NULL, NULL)) == -1) {
			printf("Error accept(): %s(%d)\n", strerror(errno), errno);
			continue;
		}
		//发送字符串到socket,问候信息
        char greet_sentence[]="220 Anonymous FTP server ready.\r\n";
        int len= strlen(greet_sentence);
        send_message(connfd,greet_sentence,len);
        printf("wait client\n");
        receive_message(connfd,sentence,&len);
        printf("%s\n",sentence);
        if(1||strcmp(sentence,"USER anonymous")==0){
            char verify_sentence[]="331 Guest login ok\r\n";
            send_message(connfd,verify_sentence,strlen(verify_sentence));
        }
        printf("wait client\n");
        receive_message(connfd,sentence,&len);
        printf("%s %d",sentence,len);
        close(connfd);
        break;
	}
	close(listenfd);
}

