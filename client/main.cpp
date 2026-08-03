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

const int MAXLEN = 1024 * 1024; //1MB
struct Response{
    uint32_t status = 0;
    std::vector<uint8_t> data;
};
enum{
    RES_OK = 0,
    RES_ERR = 1,
    RES_NX = 2
};
void alert_msg(std::string msg){
    std::cout <<"Error: "<< msg << std::endl;
}
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
int readfull(int fd, uint8_t *buf, size_t n){
    ssize_t rsize;
    while(n > 0){
        rsize = read(fd, buf, n);
        if(rsize < 0){
            return -1;
        }
        else if(rsize == 0){
            alert_msg("server closed connectoin.");
            return -2;
        }
        assert((size_t)rsize <= n);
        n -= (size_t)rsize;
        buf += rsize;
    }
    return 0;
}
static void parse_response(uint8_t *data, Response &res, int n){
    uint32_t len;
    uint8_t* end = data + n; 
    (void)read_u32(data, end, &len);
    (void)read_u32(data, end, &res.status);
    res.data.resize(n);
    (void)read_str(data, end, len - 4, &res.data[0]);
}
static std::string eval_status(int i){
    switch(i){
        case RES_OK:    return "RES_OK";
        case RES_ERR:   return "RES_ERR";
        case RES_NX:    return "RES_NX";
        default:    return "INVALID";
    }
}
static void print_response(struct Response &res){
    std::cout << "status: " << eval_status((int)res.status) << std::endl;
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
template <typename T> static void print_vector(std::vector<T> vec){
    std::cout << "[ ";
    for(auto it = vec.begin(); it != vec.end(); it++){
        std::cout << *it << " "; 
    }
    std::cout << "] " << std::endl;
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
    uint8_t rbuf[4 + MAXLEN] = {0}; 
    int err = readfull(fd, rbuf, 4);
    if(err){
        alert_msg((errno == 0)? "read(): EOF":"read() error");
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
    parse_response((uint8_t*)rbuf, res, len+4);
    print_response(res);
    return 0;
}

int connect_server(const char *host, const char *port) {
    struct addrinfo hints = {};
    hints.ai_family = AF_INET;        // IPv4
    hints.ai_socktype = SOCK_STREAM;  // TCP

    struct addrinfo *result = nullptr;

    int rv = getaddrinfo(host, port, &hints, &result);
    if (rv != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(rv) << '\n';
        return -1;
    }

    int fd = -1;

    for (struct addrinfo *p = result; p != nullptr; p = p->ai_next) {
        fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd == -1)
            continue;

        if (connect(fd, p->ai_addr, p->ai_addrlen) == 0)
            break;  // Success

        close(fd);
        fd = -1;
    }

    freeaddrinfo(result);

    return fd;
}
int main(int argc, char** argv) {
    if(argc != 3){
        die("invalid argument");
    }
    int fd = connect_server(argv[1], argv[2]);
    if(fd < 0){
        die("connect_server failure");
    }


    while(true){
        std::vector<std::string> cmd;
        std::string token;
        while(std::cin >> token){
            if(token == ";") break;
            cmd.push_back(token);
        }
        if(cmd.empty()){
            break;
        }
        if(cmd.front() == "exit"){
            break;
        }
        uint8_t query[MAXLEN] = {};
        int len = make_query(cmd, query);
        query_send(fd, query, len);
        int err = query_recv(fd);
        std::cout << "recv error: "<<err<<std::endl;
    }
    
    close(fd);

    return 0;
}
