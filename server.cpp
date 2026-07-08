#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <poll.h>
#include <fcntl.h>


struct Conn{
    int fd;
    bool want_read;
    bool want_write;
    bool want_close;
    std::vector<uint8_t> incoming;
    std::vector<uint8_t> outgoing;
};

void msg(std::string msg);
static void die(const char *msg);
static void fd_set_nb(int fd);
static Conn* handle_accept(int fd);
static void handle_close(Conn *conn);
static void handle_read(Conn *conn);
static void handle_write(Conn *conn);
static void buf_append(std::vector<uint8_t> &dest_buf, const uint8_t* sr_buf,  size_t n);
static void buf_consume(std::vector<uint8_t> &buf, size_t n);
static void try_one_request(Conn *conn);

int main(){
    int fd, rv;
    int const PORT = 1234;
    int val = 1;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1){
        die("socket failure");
    }
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(0);
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

void msg(std::string msg){
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
        msg("accept fail");
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
    uint8_t buf[64 * 1024]; //64kb
    int rv = read(conn->fd, buf, sizeof(buf));
    if(rv <= 0){
        msg("read error/connectino closed by the client");
        conn->want_close = true; 
        return;
    }
    buf_append(conn->incoming, buf, (size_t)rv );

    try_one_request(conn);

    if(conn->outgoing.size() > 0){
        conn->want_read = false;
        conn->want_write = true;
    }
}

static void handle_write(Conn * conn){
    assert(conn->outgoing.size() > 0);             //NEW: use assert helps debug impossible situations
    int rv = write(conn->fd, conn->outgoing.data(), conn->outgoing.size());
    if(rv < 0){                         //write sets errno
        msg("write error");
        conn->want_close = true;
        return;
    }
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
static void try_one_request(Conn *conn){
    const int MAXLEN = 4096;
    uint8_t buf[MAXLEN];
    if(conn->incoming.size() < 4){
        return;                          //wants more read
    }
    uint32_t len;
    memcpy(&len, conn->incoming.data(), 4);
    if(len > MAXLEN){
        conn->want_close = true;        //protocol error therefore want close
        return;
    }
    if(conn->incoming.size() < 4+len){
        return;                             //wants more read
    }
    uint8_t *msg = &conn->incoming[4];


    //simply echo the msg back as of now
    //append reply to outgoing buffer
    buf_append(conn->outgoing, (const uint8_t*)&len, 4);
    buf_append(conn->outgoing, msg, len);

    //remove from incoming buffer
    buf_consume(conn->incoming, 4 +len);
    return;
}