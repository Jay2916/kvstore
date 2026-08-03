#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <poll.h>
#include <fcntl.h>
#include "../include/hashtable.h"
#include <vector>
#include <stdlib.h>

//upgrades to do:
//1.poll -> epoll
//2.better buffer management
//3.combine do_request() + make_response() ???


#define container_of(ptr, T, member) \
    ((T *)( (char *)ptr - offsetof(T, member) ))

enum{
    RES_OK = 0,
    RES_ERR = 1,
    RES_NX = 2
};
struct Conn{
    int fd;
    bool want_read;
    bool want_write;
    bool want_close;
    std::vector<uint8_t> incoming;
    std::vector<uint8_t> outgoing;
};

struct Response{
    uint32_t status = 0;
    std::vector<uint8_t> data;
};

struct Entry{
    HNode node;
    std::string key;
    std::string value;
};

static struct {
    HMap db;
}g_data;

const int MAXLEN = 1024 * 1024; //1MB
const int MAXNSTR = 10;


void alert_msg(std::string msg);
static void die(const char *msg);
static void fd_set_nb(int fd);

static Conn* handle_accept(int fd);
static void handle_close(Conn *conn);
static void handle_read(Conn *conn);
static void handle_write(Conn *conn);

static void buf_append(std::vector<uint8_t> &dest_buf, const uint8_t* sr_buf,  size_t n);
static void buf_consume(std::vector<uint8_t> &buf, size_t n);

static bool try_one_request(Conn *conn);
static void make_response(struct Response &res, std::vector<uint8_t> &out);
static void do_request(std::vector<std::string> &cmd, struct Response &out);
static int parse_request(uint8_t *msg, int32_t len, std::vector<std::string> &out);

static bool read_str(uint8_t *&curr, uint8_t* end, uint32_t len, std::string &out);
static bool read_u32(uint8_t *&curr, uint8_t* end, uint32_t* out);

static void do_get(std::string &key, Response &out);
static void do_del(std::string &key, Response &out);
static void do_set(std::string &key, std::string &value, Response &out);



int main(int argc , char** argv){
    int fd, rv;
    if(argc != 2){
        die("invalid arguments");
    }
    int const PORT = atoi(argv[1]);
    std::cout << "Starting on PORT " << PORT << std::endl;

    int val = 1;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1){
        die("socket failure");
    }
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if(rv == -1){
        die("bind failure");
    }
    rv = listen(fd, SOMAXCONN);
    if(rv == -1){
        die("listen failure");
    }
    std::vector<Conn*> fd2conn;
    std::vector<struct pollfd> poll_args;
    while(true){
        //initialize poll_args
        poll_args.clear();
        struct pollfd listenpfd = {fd, POLLIN, 0};
        poll_args.push_back(listenpfd);
        for(Conn* conn : fd2conn){
            if(!conn){
                continue;
            }
            struct pollfd pfd = {conn->fd, POLLERR, 0};
            if(conn->want_read){
                pfd.events |= POLLIN;
            }
            if(conn->want_write){
                pfd.events |= POLLOUT;
            }
            poll_args.push_back(pfd);

        }

        //waiting for readiness using poll()
        int rv = poll(poll_args.data(), (nfds_t)poll_args.size(),-1);
        if(rv < 0 && errno == EINTR ){
            continue;
        }
        if(rv < 0){
            die("poll");
        }

        //check the listening pfd
        if(poll_args[0].revents){
            if(Conn* conn = handle_accept(poll_args[0].fd)){
                if(fd2conn.size() <= (size_t)conn->fd){
                    fd2conn.resize(conn->fd + 1);
                }
                fd2conn[conn->fd] = conn;
            }

        }

        //check Conneection pfds
        for(int i = 1; i < poll_args.size(); i++){
            struct pollfd pfd = poll_args[i];
            Conn* conn = fd2conn[pfd.fd];
            short ready = pfd.revents;
            if(pfd.revents & POLLIN){
                handle_read(conn);
            }
            if(pfd.revents & POLLOUT){
                handle_write(conn);
            }
            if((ready & POLLERR) || conn->want_close){
                handle_close(conn);
                fd2conn[conn->fd] = nullptr;
                delete conn;
            }
        }

    }

    //stop server
    close(fd);

}

void alert_msg(std::string msg){
    std::cout <<"Alert: "<< msg << std::endl;
}

static void die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

static void fd_set_nb(int fd) {
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

static Conn* handle_accept(int fd){
    struct sockaddr_in client_addr = {};
    socklen_t addrlen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr*)&client_addr, &addrlen);
    if(connfd == -1){
        alert_msg("accept fail");
    }
    fd_set_nb(connfd);
    Conn* conn = new Conn();
    conn->fd = connfd;
    conn->want_read = true;
    return conn;
}

static void handle_close(Conn *conn){
    (void)close(conn->fd);
}

static void handle_read(Conn * conn){
    uint8_t buf[MAXLEN]; 
    int rv = read(conn->fd, buf, sizeof(buf));
    
    if(rv <= 0){
        if(rv == 0) alert_msg("connection closed by the client\n\n");
        else alert_msg("read() error");
        conn->want_close = true; 
        return;
    }
    std::cout << "read() returned: " << rv << std::endl;
    buf_append(conn->incoming, buf, (size_t)rv );
    
    while(try_one_request(conn)); //optimized for batch requests

    if(conn->outgoing.size() > 0){
        conn->want_read = false;
        conn->want_write = true;
        //the socket is likely ready to write in a request-reponse protocol
        //therefore avoid taking one more iteration of poll and write immediately
        //BUT due to batch processing the socket send buffer may be full so we have to check for EAGAIN to be sure
        return handle_write(conn);  //optimized
    }
}

static void handle_write(Conn * conn){
    assert(conn->outgoing.size() > 0);             //NEW: use assert helps debug impossible situations
    int rv = write(conn->fd, conn->outgoing.data(), conn->outgoing.size());
    if(rv < 0 && rv == EAGAIN){
        return; // socket not ready to write yet(due to batch processing of requests) 
    }
    if(rv < 0){                         //write sets errno
        alert_msg("write error");
        conn->want_close = true;
        return;
    }
    std::cout << "write() returned: " << rv << std::endl;
    buf_consume(conn->outgoing, (size_t)rv);

    if(conn->outgoing.size() == 0){
        conn->want_read = true;
        conn->want_write = false;
    }
}

static void buf_append(std::vector<uint8_t> &dest_buf, const uint8_t* sr_buf,  size_t n){
    dest_buf.insert(dest_buf.end(), sr_buf, sr_buf + n);
}

static void buf_consume(std::vector<uint8_t> &buf, size_t n){
    buf.erase(buf.begin(), buf.begin() + n);
}

static bool try_one_request(Conn *conn){
    
    //uint8_t buf[MAXLEN];
    if(conn->incoming.size() < 4){
        return false;                          //wants more read
    }
    uint32_t len;
    memcpy(&len, conn->incoming.data(), 4);
    if(len > MAXLEN){
        alert_msg("protocol len error");
        conn->want_close = true;        //protocol error therefore want close
        return false;
    }
    if(conn->incoming.size() < 4+len){
        return false;                             //wants more read
    }
    uint8_t *msg = &conn->incoming[4];
    std::vector<std::string> cmd;
    struct Response res = {};
    if(parse_request(msg, len, cmd) < 0){
        alert_msg("parse error");
        conn->want_close = true;
        return false;
    }
    buf_consume(conn->incoming, len+4);
    do_request(cmd, res);
    make_response(res, conn->outgoing);
    return true;
}

static int parse_request(uint8_t *msg, int32_t len, std::vector<std::string> &out){
    uint32_t nstr;
    uint8_t* end = msg + len;
    if(!read_u32(msg, end, &nstr)){
        return -1;
    }
    if(nstr > MAXNSTR){
        return -1;
    }
    
    while(out.size() < (size_t)nstr){
        uint32_t strlen = 0;
        if(!read_u32(msg, end, &strlen)){
            return -1;
        }
        std::string str;
        if(!read_str(msg, end, strlen, str)){
            return -1;
        }
        out.push_back(str);
    }
    if(msg != end){
        return -1;      //trailing garbage
    }
    return 0;
}

static bool read_u32(uint8_t *&curr, uint8_t* end, uint32_t* out){
    if(curr + 4 > end){
        return false;
    }
    memcpy(out, curr, 4);
    curr += 4;
    return true;
}

static bool read_str(uint8_t *&curr, uint8_t* end, uint32_t len, std::string &out){
    if(curr + len > end){
        return false;
    }
    out.assign(curr, curr + len);
    curr += len ;
    return true;
}

static bool entry_eq(HNode *hnode1, HNode *hnode2){
    Entry *e1 = container_of(hnode1, Entry, node);
    Entry *e2 = container_of(hnode2, Entry, node);
    return e1->key == e2->key;

}

static void do_request(std::vector<std::string> &cmd, struct Response &out){
    if(cmd.size() == 2 && cmd[0] == "get"){
        do_get(cmd[1], out);
    }
    else if (cmd.size() == 2 && cmd[0] == "del"){
        do_del(cmd[1], out);
    }
    else if(cmd.size() == 3 && cmd[0] == "set"){
        do_set(cmd[1], cmd[2], out);
    }
    else{
        out.status = RES_ERR;
    }
}

static void make_response(struct Response &res, std::vector<uint8_t> &out){
    uint32_t len = sizeof(res.status) + (uint32_t)res.data.size();
    buf_append(out, (const uint8_t*)&len, sizeof(len));
    buf_append(out, (const uint8_t*)&res.status, sizeof(res.status));
    buf_append(out, res.data.data(), res.data.size());
}

static uint64_t str_hash(std::string str){ //fnv-1a hash function
    uint64_t hash = 14695981039346656037ULL; // FNV offset basis
    for (size_t i = 0; i < str.length(); i++) {
        hash ^= (uint64_t)str[i];
        hash *= 1099511628211ULL;            // FNV prime
    }
    return hash;
}

static void do_get(std::string &key, Response &out){
    alert_msg("doing get");
    Entry lookup{};
    lookup.key = key;
    lookup.node.hcode = str_hash(key);
    HNode *node = hm_lookup(&(g_data.db), &lookup.node, entry_eq);
    if(node){
        out.status = RES_OK;
        Entry *e = container_of(node, Entry, node);
        out.data.assign( e->value.begin(), e->value.end());
    }
    else{
        out.status = RES_NX;
        out.data = {};
    }

}

static void do_set(std::string &key, std::string &value, Response &out){
    alert_msg("doing set");
    Entry lookup{};
    lookup.key = key;
    lookup.node.hcode = str_hash(key);
    HNode *node = hm_lookup(&g_data.db, &lookup.node, entry_eq);
    if(node){
        container_of(node, Entry, node)->value = std::move(value);
    }
    else{
        Entry *e = new Entry();
        e->node.hcode = lookup.node.hcode;
        e->node.next = NULL;
        e->key = std::move(key);
        e->value = std::move(value);
        hm_insert(&g_data.db, &e->node);
    }
    out.status = RES_OK;
    out.data = {};
    

}

static void do_del(std::string &key, Response &out){
    alert_msg("doing del");
    Entry lookup{};
    lookup.key = key;
    lookup.node.hcode = str_hash(key);
    HNode *node = hm_delete(&g_data.db, &lookup.node, entry_eq);
    if(node){
        out.status = RES_OK;
        out.data = {};
        Entry *e = container_of(node, Entry, node);
        delete e;
    }
    else{
        out.status = RES_NX;
        out.data = {};
    }
}