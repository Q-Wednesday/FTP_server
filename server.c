
#define LISTENPORT 21
#include "core.h"
extern int running;
int main(int argc, char **argv) {
    ServerParams params={argc,argv};
    pthread_t main_thread;
    pthread_create(&main_thread,NULL,init_server,&params);
    char message[100];
    gets(message);
    if(strcasecmp(message,"quit")==0) {
        running = 0;
        printf("quit\n");
    }
    return 0;
}

