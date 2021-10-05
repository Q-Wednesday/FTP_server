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
    init_server();

}

