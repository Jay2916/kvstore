#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <assert.h>
#include <fcntl.h>

#include "../include/hashtable.hpp"
#include "../include/helper.hpp"
#include "../include/server.hpp"
#include "../include/ringbuffer.hpp"
#include "../include/codec.hpp"
#include "../include/RequestDispatcher.hpp"

#define MIN_WRITABLE 0
//upgrades to do:
//1. change conn* to a smart pointer
//2.implement WAL -> batching -> snapshots of database
//3. remove assert and close connection when serialization helpers fail, instead of crashing the server
//4. gracefull shutdown

//what happens when either of the buffer is full?
//incoming: make want_read false
//outgoing: make want_write true
#define MAX_BUF 1024
struct Conn{
    int fd;
    bool want_read;
    bool want_write;
    bool want_close;
    RingBuffer<std::byte> incoming;
    RingBuffer<std::byte> outgoing;
    struct PendingRespose{
        std::vector<std::byte> data;
        size_t offset = 0;
    } pending_response;
    Conn(int fd, size_t max_capacity)
        :fd(fd), want_read(true), want_write(false), want_close(false), incoming(max_capacity), outgoing(max_capacity){}
};

KVserver::KVserver(uint16_t const port, RequestDispatcher& rd)
    :PORT(port), requestDispatcher(rd){
    running = true;
}

KVserver::~KVserver(){
    this->stop();
}
void KVserver::start_listening(){
    int rv;
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
}
void KVserver::start(){
    int rv;
    std::cout << "Starting on PORT " << PORT << std::endl;
    start_listening();
    running = true;

    epfd = epoll_create1(0);
    if(epfd == -1)  die("epoll_create1() fail");

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = nullptr;
    if(epoll_ctl(epfd, EPOLL_CTL_ADD, this->fd, &ev) < 0){
        display_error("epoll_ctl(ADD LISTENING) fail");
    }
    
    while(running){

        rv = epoll_wait(epfd, events.data(), static_cast<int>(events.size()),-1);
        if(rv < 0 && errno == EINTR ){
            continue;
        }
        if(rv < 0){
            die("poll");
        }

        //check the listening pfd
        for(int i = 0; i < rv; i++){
            epoll_event& ev = events[i];

            if(ev.data.ptr == nullptr){
                if(handle_accept(this->fd) == -1){
                    display_error("handle_accept() fail");
                }
                continue;
            }
            Conn* conn = static_cast<Conn*>(ev.data.ptr);
           
            if((ev.events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) || conn->want_close){
                handle_close(conn);
                continue;
            }
            if(ev.events & EPOLLIN){
                handle_read(conn);
                if(conn->want_close){
                    handle_close(conn);
                    continue;
                }
            }
            if(ev.events & EPOLLOUT){
                handle_write(conn);
                if(conn->want_close){
                    handle_close(conn);
                    continue;
                }
            }
        }


    }

}
void KVserver::stop(){
    running = false;
    close(fd);
    close(epfd);
    fd = -1;
    epfd = -1;
}
int KVserver::handle_accept(int fd){
    struct sockaddr_in client_addr = {};
    socklen_t addrlen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr*)&client_addr, &addrlen);
    if(connfd == -1){
        alert_msg("accept fail");
        return -1;
    }
    if(fd_set_nb(connfd) == -1){
        return -1;
    }
    Conn* conn = new Conn(connfd, MAX_BUF);
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = conn;
    if(epoll_ctl(epfd, EPOLL_CTL_ADD, conn->fd, &ev) < 0){
        display_error("epoll_ctl(ADD) fail");
        return -1;
    }
    return 0;
}

void KVserver::handle_close(Conn* conn){
    epoll_ctl(this->epfd, EPOLL_CTL_DEL, conn->fd, 0);
    (void)close(conn->fd);
    delete conn;
}

void KVserver::handle_read(Conn* conn){
    std::byte buf[MAX_QLEN];
    size_t writable = conn->incoming.writable();
    if(writable == 0){
        return;
    }
    ssize_t rv = read(conn->fd, buf, writable);
    if(rv == 0){
        alert_msg("connection closed by the client\n\n");
        conn->want_close = true;
        return;
    }
    else if(rv < 0){
        if(errno == EAGAIN || errno == EWOULDBLOCK){
            return;
        }
        else {
            alert_msg("read() error");
        }
        conn->want_close = true; 
        return;
    }
    std::cout << "read() returned: " << rv << std::endl;

    size_t n = conn->incoming.append(std::span(buf,static_cast<size_t>(rv)));
    if(n < static_cast<size_t>(rv)){
        alert_msg("handle_read: incoming buffer full, read bytes rejected");
    }
    

    while(try_one_request(conn)); //optimized for batch requests

    if(!conn->outgoing.empty()){
        conn->want_read = false;
        conn->want_write = true;
        update_epoll_event(conn);
        //the socket is likely ready to write in a request-reponse protocol
        //therefore avoid taking one more iteration of poll and write immediately
        //BUT due to batch processing the socket send buffer may be full so we have to check for EAGAIN to be sure
        return handle_write(conn);  //optimized
    }
}

void KVserver::handle_write(Conn * conn){
    if(conn->outgoing.empty()){
        return;
    }          

    int rv = write(conn->fd, conn->outgoing.front(), conn->outgoing.contigious_readable());

    //if i want to write in a single call from ringbuffer, i will have to straighten up the data
    //or, i could call write only on front->last element (implemented)

    if(rv < 0){
        if(errno == EAGAIN || errno == EWOULDBLOCK){
            return;
        }
        alert_msg("write error");
        conn->want_close = true;
        return;
    }

    std::cout << "write() returned: " << rv << std::endl;

    conn->outgoing.consume(static_cast<size_t>(rv));

    if(conn->outgoing.empty()){
        conn->want_read = true;
        conn->want_write = false;
        update_epoll_event(conn);
    }
}

//RequestDispatcher
bool KVserver::try_one_request(Conn *conn){

    //outgoing buffer is full and threrfore needs to be sent before trying new request
    if (conn->pending_response.offset < conn->pending_response.data.size()) {
        auto remaining = std::span<const std::byte>( conn->pending_response.data).subspan(conn->pending_response.offset);
        size_t n = conn->outgoing.append(remaining);
        conn->pending_response.offset += n;
        //the socket must write before trying new requests
        return false;
    }
    if(conn->incoming.readable() < 4){
        return false;                          //wants more read
    }
    uint32_t len = peek_u32(conn->incoming);

    if(len > MAX_QLEN){
        alert_msg("protocol len error");
        conn->want_close = true;        //protocol error therefore want close
        return false;
    }
    if(conn->incoming.getsize() < len){
        return false;                             //wants more read
    }

    
    std::vector<std::byte> buf = conn->incoming.peek(len);
    Query query = Codec::decode_query(buf);
    Response response = this->requestDispatcher.dispatch(query);    
    conn->pending_response.data = Codec::encode_response(response);

    // if(parse_request(std::span(buf), cmd, input) == -1){
    //     alert_msg("parse error");
    //     conn->want_close = true;
    //     return false;
    // }
    
    conn->incoming.consume(static_cast<size_t>(len));
    // if(do_request(cmd, input, conn) < 0){
    //     alert_msg("invalid request");
    //     conn->want_close = true;
    //     return false;
    // }
    
    conn->pending_response.offset = conn->outgoing.append(conn->pending_response.data);
    return true;
}
// //codec
// int KVserver::parse_request(std::span<const std::byte> reader, Command &cmd, std::vector<std::vector<std::byte>> &out){
//     cmd = static_cast<Command>(sread_u8(reader));
//     uint32_t n = sread_u32(reader);
//     if(n > MAXNSTR){
//         return -1;      //protocol error
//     }
//     while(out.size() < (size_t)n){
//         out.push_back(sread(reader));
//     }
//     if (!reader.empty())
//         return -1;
//     return 0;
// }

// //RequestDispatcher
// int KVserver::do_request(Command cmd, std::vector<std::vector<std::byte>> &input, Conn* conn){
//     Response res = {};
//     switch(cmd){
//         case Command::GET:
//             if(input.size() != 1) return -1;
//             do_get(input[0], res);
//             break;
//         case Command::SET:
//             if(input.size() != 2) return -1;
//             do_set(input[0], input[1], res);
//             break;
//         case Command::DEL:
//             if(input.size() != 1) return -1;
//             do_del(input[0], res);
//             break;
//         default:
//             res.status = Status::RES_ERR;
//     }
//     serialize_response()
    
//     return 0;

// }
//StorageEngine
// uint64_t KVserver::hash_bytes(std::span<const std::byte> data) {
//     uint64_t hash = 14695981039346656037ULL;
//     for (std::byte b : data) {
//         hash ^= std::to_integer<uint8_t>(b);
//         hash *= 1099511628211ULL;
//     }
//     return hash;
// }
//StorageEngine
// void KVserver::do_get(std::vector<std::byte> &key, Response &out){
//     alert_msg("doing get");
//     Entry lookup{};
//     lookup.key = key;
//     lookup.node.hcode = hash_bytes(key);
//     HNode *node = hm_lookup(&(g_data.db), &lookup.node, entry_eq);
//     if(node){
//         out.status = Status::RES_OK;
//         Entry *e = container_of(node, Entry, node);
//         out.data.assign( e->value.begin(), e->value.end());
//     }
//     else{
//         out.status = Status::RES_NX;
//         out.data = {};
//     }

// }
//StorageEngine
// void KVserver::do_set(std::vector<std::byte> &key, std::vector<std::byte> &value, Response &out){
//     alert_msg("doing set");
//     Entry lookup{};
//     lookup.key = key;
//     lookup.node.hcode = hash_bytes(key);
//     HNode *node = hm_lookup(&g_data.db, &lookup.node, entry_eq);
//     if(node){
//         container_of(node, Entry, node)->value = std::move(value);
//     }
//     else{
//         Entry *e = new Entry();
//         e->node.hcode = lookup.node.hcode;
//         e->node.next = NULL;
//         e->key = std::move(key);
//         e->value = std::move(value);
//         hm_insert(&g_data.db, &e->node);
//     }
//     out.status = Status::RES_OK;
//     out.data = {};
// }
//StorageEngine
// void KVserver::do_del(std::vector<std::byte> &key, Response &out){
//     alert_msg("doing del");
//     Entry lookup{};
//     lookup.key = key;
//     lookup.node.hcode = hash_bytes(key);
//     HNode *node = hm_delete(&g_data.db, &lookup.node, entry_eq);
//     if(node){
//         out.status = Status::RES_OK;
//         out.data = {};
//         Entry *e = container_of(node, Entry, node);
//         delete e;
//     }
//     else{
//         out.status = Status::RES_NX;
//         out.data = {};
//     }
// }

void KVserver::update_epoll_event(Conn* conn) {
    epoll_event ev{};

    ev.data.ptr = conn;

    if (conn->want_read)
        ev.events |= EPOLLIN;

    if (conn->want_write)
        ev.events |= EPOLLOUT;

    if (epoll_ctl(epfd, EPOLL_CTL_MOD, conn->fd, &ev) < 0) {
        display_error("epoll_ctl(MOD) fail");
    }
}