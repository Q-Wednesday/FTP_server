
#define LISTENPORT 21
#include "core.h"
int main(int argc, char **argv) {
    int state=init_server(argc, argv);
    switch (state) {
        case -1:
            printf("Argument Parsed Error\n");
            break;
        case 1:
            printf("Socket error\n");
            break;
        default:
            break;
    }
    return state;
}

