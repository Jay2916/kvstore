#include <iostream>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <assert.h>
#include <string.h>
#include <vector>
#include <sys/types.h>
#include <netdb.h>

static void alert_msg(std::string msg){
    std::cout <<"Error: "<< msg << std::endl;
}
static void die(const char *msg) {

    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}


