
#define LISTENPORT 21
#include "core.h"
extern int running;

int main(int argc, char **argv) {
    ServerParams params={argc,argv};
    init_server(&params);

    return 0;
}

