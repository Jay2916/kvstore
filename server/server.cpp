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


//upgrades to do:
//0.take entry_eq and Entry itself from user, making it customizable
//1.poll -> epoll
//2.better buffer management (switch to custom Ringbuffer)
//3.comvert key and value input to be opaque data i.e. uint8_t?? then intrusive data structure wont have any point
//implement persistance (first make a snapshot -> then also implement WAL)


#define container_of(ptr, T, member) \
    ((T *)( (char *)ptr - offsetof(T, member) ))


bool entry_eq(HNode *hnode1, HNode *hnode2){
    Entry *e1 = container_of(hnode1, Entry, node);
    Entry *e2 = container_of(hnode2, Entry, node);
    return e1->key == e2->key;
}



KVserver::KVserver(uint16_t const port)
    :PORT(port){
    running = true;
}

KVserver::~KVserver(){
    close(fd);
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
    while(running){
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
        rv = poll(poll_args.data(), (nfds_t)poll_args.size(),-1);
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

}
void KVserver::stop(){
    if(running){
        running = false;
        this->~KVserver();
    }
}
Conn* KVserver::handle_accept(int fd){
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

void KVserver::handle_close(Conn *conn){
    (void)close(conn->fd);
}

void KVserver::handle_read(Conn * conn){
    uint8_t buf[MAX_QLEN]; 
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

void KVserver::handle_write(Conn * conn){
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

void KVserver::buf_append(std::vector<uint8_t> &dest_buf, const uint8_t* sr_buf,  size_t n){
    dest_buf.insert(dest_buf.end(), sr_buf, sr_buf + n);
}

void KVserver::buf_consume(std::vector<uint8_t> &buf, size_t n){
    buf.erase(buf.begin(), buf.begin() + n);
}

bool KVserver::try_one_request(Conn *conn){
    
    //uint8_t buf[MAXLEN];
    if(conn->incoming.size() < 4){
        return false;                          //wants more read
    }
    std::span<const uint8_t> reader = conn->incoming;
    uint32_t len = vread_u32(reader);
    if(len > MAX_QLEN){
        alert_msg("protocol len error");
        conn->want_close = true;        //protocol error therefore want close
        return false;
    }
    if(conn->incoming.size() < 4+len){
        return false;                             //wants more read
    }

    Command cmd;
    std::vector<std::string> input;

    if(parse_request(reader, cmd, input) < 0){
        alert_msg("parse error");
        conn->want_close = true;
        return false;
    }
    buf_consume(conn->incoming, len+4);
    do_request(cmd, input, conn->outgoing);
    return true;
}

int KVserver::parse_request(std::span<const uint8_t> reader, Command &cmd, std::vector<std::string> &out){
    cmd = static_cast<Command>(vread_u8(reader));
    uint32_t nstr = vread_u32(reader);
    if(nstr > MAXNSTR){
        return -1;      //protocol error
    }
    while(out.size() < (size_t)nstr){
        std::string str = vread_str(reader);
        out.push_back(str);
    }
    return 0;
}


void KVserver::do_request(Command cmd, std::vector<std::string> &input, std::vector<uint8_t> &out){
    Response res = {};
    switch(cmd){
        case Command::GET:
            do_get(input[0], res);
            break;
        case Command::SET:
            do_set(input[0], input[1], res);
            break;
        case Command::DEL:
            do_del(input[0], res);
            break;
        default:
            res.status = Status::RES_ERR;
    }
    uint32_t tlen = sizeof(Status) + 4 + res.data.size();
    std::vector<uint8_t> buffer(tlen + 4);
    std::span<uint8_t> writer = buffer;

    vwrite_u32(writer, static_cast<uint32_t>(tlen));
    vwrite_u8(writer, static_cast<uint8_t>(res.status));
    vwrite_str(writer, res.data);

    assert(writer.empty());

    buf_append(out, buffer.data(), buffer.size());
}
uint64_t KVserver::str_hash(std::string str){ //fnv-1a hash function
    uint64_t hash = 14695981039346656037ULL; // FNV offset basis
    for (size_t i = 0; i < str.length(); i++) {
        hash ^= (uint64_t)str[i];
        hash *= 1099511628211ULL;            // FNV prime
    }
    return hash;
}

void KVserver::do_get(std::string &key, Response &out){
    alert_msg("doing get");
    Entry lookup{};
    lookup.key = key;
    lookup.node.hcode = str_hash(key);
    HNode *node = hm_lookup(&(g_data.db), &lookup.node, entry_eq);
    if(node){
        out.status = Status::RES_OK;
        Entry *e = container_of(node, Entry, node);
        out.data.assign( e->value.begin(), e->value.end());
    }
    else{
        out.status = Status::RES_NX;
        out.data = {};
    }

}

void KVserver::do_set(std::string &key, std::string &value, Response &out){
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
    out.status = Status::RES_OK;
    out.data = {};
}

void KVserver::do_del(std::string &key, Response &out){
    alert_msg("doing del");
    Entry lookup{};
    lookup.key = key;
    lookup.node.hcode = str_hash(key);
    HNode *node = hm_delete(&g_data.db, &lookup.node, entry_eq);
    if(node){
        out.status = Status::RES_OK;
        out.data = {};
        Entry *e = container_of(node, Entry, node);
        delete e;
    }
    else{
        out.status = Status::RES_NX;
        out.data = {};
    }
}