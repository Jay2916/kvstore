#include <iostream>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <assert.h>
#include <string.h>

struct Response{
    uint32_t status = 0;
    std::vector<uint8_t> data;
};
static void die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}
static bool read_u32(uint8_t *&curr, uint8_t* end, uint32_t* out){
    if(curr + 4 > end){
        return false;
    }
    memcpy(out, curr, 4);
    curr += 4;
    return true;
}
static bool read_str(uint8_t *&curr, uint8_t* end, uint32_t len, uint8_t* out){
    if(curr + len > end){
        return false;
    }
    memcpy(out, curr, len);
    curr += len ;
    return true;
}
const int MAXLEN = 1024 * 1024; //1MB
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
static void parse_response(uint8_t *data, Response &res){
    uint32_t len;
    uint8_t* end = data + 1024; //temp 
    (void)read_u32(data, end, &len);
    (void)read_u32(data, end, &res.status);
    res.data.resize(100);
    (void)read_str(data, end, len - 4, &res.data[0]);
}

static void print_response(struct Response &res){
    std::cout << "status: " << (int)res.status << std::endl;
    std::cout << "data: " << std::string(res.data.begin(), res.data.end()) << std::endl;
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
void alert_msg(std::string msg){
    std::cout <<"Error: "<< msg << std::endl;
}
int query_send(int fd, const uint8_t* text, int n){
    uint32_t len = n;
    if(len > MAXLEN){
        alert_msg("query too long");
        return -1;
    }
    char wbuf[4 + MAXLEN];
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], text, len);
    int err;
    err = writeall(fd, wbuf, 4 + len);
    if(err){
        alert_msg("write() error");
        return err;
    }
    std::cout <<"Sent lenght: "<< len+4 << std::endl;
    return 0;
}
static bool write_u32(uint8_t *&curr, uint8_t* end, uint32_t in){
    if(curr + 4 > end){
        return false;
    }
    memcpy(curr, &in, sizeof(in));
    curr += 4;
    return true;
}
static bool write_str(uint8_t *&curr, uint8_t* end, const uint8_t *in, uint32_t len){
    if(curr + len > end){
        return false;
    }
    memcpy(curr, in, len);
    curr += len;
    return true;

}
static int make_query(std::vector<std::string> cmd, uint8_t *out){
    uint8_t * start = out;
    uint32_t nstr = cmd.size();
    uint8_t *end = out + MAXLEN;
    if(!write_u32(out, end, nstr)){
        return -1;
    }
    for(std::string str: cmd){
        uint32_t strlen = str.length();
        if(!write_u32(out, end, strlen)){
            return -1;
        }
        if(!write_str(out, end, (const uint8_t*)str.data(), strlen)){
            return -1;
        }
    }
    return out - start;

}

int query_recv(int fd){
    uint32_t len;
    errno = 0;
    char rbuf[4 + MAXLEN];
    int err = readfull(fd, rbuf, 4);
    if(err){
        alert_msg((errno == 0)? "EOF":"read() error");
        return err;
    }
    memcpy(&len, rbuf, 4);
    if(len > MAXLEN){
        alert_msg("too long");
        return -1;
    }
    err = readfull(fd, &rbuf[4], len);
    if(err){
        alert_msg("read() error");
        return err;
    }
    std::cout << "Received length: " << len+4 << std::endl;
    struct Response res = {};
    parse_response((uint8_t*)rbuf, res);
    print_response(res);
    return 0;
}


int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(4444);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);  // 127.0.0.1
    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv) {
        die("connect");
    }
    ///////////////////////////////////////////////////////

    std::vector<std::string> cmd = {"get", "hi"};
    uint8_t query[MAXLEN];
    int len = make_query(cmd, query);
    query_send(fd, query, len);
    query_recv(fd);


    ///////////////////////////////////////////////////////
    close(fd);

    return 0;
}
