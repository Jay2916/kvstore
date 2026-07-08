#include <iostream>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <assert.h>
#include <string.h>

static void die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}
int readfull(int fd, char *buf, size_t n){
    ssize_t rsize;
    while(n > 0){
        rsize = read(fd, buf, n);
        if(rsize <= 0){
            return -1;
        }
        assert((size_t)rsize <= n);
        n -= (size_t)rsize;
        buf += rsize;
    }
    return 0;
}
int writeall(int fd, char*buf, size_t n){
    ssize_t rsize;
    while(n > 0){
        rsize = write(fd, buf, n);
        if(rsize <= 0){
            return -1;
        }
        assert((size_t)rsize <= n);
        n -= rsize;
        buf += rsize;
    }
    return 0;
}
void msg(std::string msg){
    std::cout <<"Error: "<< msg << std::endl;
}
int query(int fd, const char* text){
    const int MAXLEN = 4096;
    uint32_t len = strlen(text);
    char wbuf[4 + len];
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], text, len);
    int err;
    err = writeall(fd, wbuf, 4 + len);
    if(err){
        msg("write() error");
        return err;
    }
    errno = 0;
    char rbuf[4 + MAXLEN];
    err = readfull(fd, rbuf, 4);
    if(err){
        msg((errno == 0)? "EOF":"read() error");
        return err;
    }
    memcpy(&len, rbuf, 4);
    if(len > MAXLEN){
        msg("too long");
        return -1;
    }
    err = readfull(fd, &rbuf[4], len);
    if(err){
        msg("read() error");
        return err;
    }
    rbuf[len + 4] = '\0';
    std::cout << "Received: " << &rbuf[4] <<" | Length: " << len << std::endl;
    return 0;
}
int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);  // 127.0.0.1
    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv) {
        die("connect");
    }
    int err;
    err = query(fd, "hello123");
    if(err) goto DONE;
    err = query(fd, "i am thee client");
    if(err) goto DONE;

DONE:
    close(fd);
    return 0;
}