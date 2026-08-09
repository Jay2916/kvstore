#include <iostream>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "../include/helper.h"
#include "../include/ringbuffer.h"
#include "../src/kvprotocol.cpp"
#include <unistd.h>
#include <assert.h>

class KVclient{
private:
    int fd;
    std::string serv_addr;
    uint16_t serv_port;
public:
    explicit KVclient(const std::string &addr, uint16_t port)
    : fd(-1), serv_addr(addr), serv_port(port){
        int rv = this->connect_server(serv_addr, serv_port);
        if(rv == -1){
            die("connect_server failure");
        }
    }
    ~KVclient(){
        alert_msg("Disconnected from server.");
        close(fd);
    }
    void print_success(struct addrinfo* ai){
        char host[NI_MAXHOST];
        char serv[NI_MAXSERV];
        int err = getnameinfo(ai->ai_addr, ai->ai_addrlen, host, sizeof(host), serv, sizeof(serv), NI_NUMERICHOST | NI_NUMERICSERV);
        if(err == 0)    std::cout << "Connected to " << host << ":" << serv << std::endl;
        else            alert_msg("getnameinfo error");
    }
    int connect_server(const std::string &host, const uint16_t port) {
        struct addrinfo hints = {};
        hints.ai_family = AF_UNSPEC;        // IPv4
        hints.ai_socktype = SOCK_STREAM;  // TCP

        struct addrinfo *result = nullptr;

        int rv = getaddrinfo(host.data(), std::to_string(port).data(), &hints, &result);
        if (rv != 0) {
            alert_msg(std::string("getaddrinfo: ") + gai_strerror(rv));
            return -1;
        }
        for (struct addrinfo *p = result; p != nullptr; p = p->ai_next) {
            fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if (fd == -1)
                continue;

            if (connect(fd, p->ai_addr, p->ai_addrlen) == 0){
                print_success(p);
                break;  // Success
            }
            close(fd);
            fd = -1;
        }

        freeaddrinfo(result);
        if(fd == -1) return -1;
        return 0;
    }
    Response send_query(Command cmd, std::vector<std::string> input){
        std::vector<uint8_t> out(MAX_QLEN);
        uint32_t tlen = sizeof(Command) + input.size();
        std::span<uint8_t> writer = out;
        writer = writer.subspan(4); //reserve space for total len
        vwrite_u8(writer, static_cast<uint8_t>(cmd));
        vwrite_u32(writer, static_cast<uint32_t>(input.size()));
        for(const auto& it : input){
            vwrite_str(writer, it);
            tlen += 4 + sizeof(it);
        }
        writer = out;
        vwrite_u32(writer, tlen);
        writeallwe(fd, std::span(out).first(tlen + 4));
        std::cout << "write: " << tlen + 4 << std::endl;
        return read_response();
    }
    Response read_response(){
        uint8_t len_buf[4];
        readfullwe(fd, len_buf);
        std::span<const uint8_t> len_reader = len_buf;
        size_t tlen = (vread_u32(len_reader));
        if(tlen > MAX_RLEN){
            alert_msg("Response data too long.");
            return {};
        }
        std::vector<uint8_t> in(tlen);
        readfullwe(fd, in);
        std::span<const uint8_t> reader = in;
        Response res;
        res.status = static_cast<Status>(vread_u8(reader));
        if(!reader.empty()){
            res.data = vread_str(reader);
        }
        assert(reader.empty()); //trailing garbage
        std::cout << "read: " << tlen + 4<< std::endl;
        return res;
    }

    
};

int main(){
    KVclient kvc{"localhost", (uint16_t)(1234)};
    Response res;
    std::vector<std::string> query_input;
    query_input.push_back("hi");
    //query_input.push_back("bye");
    res = kvc.send_query(Command::GET, query_input);
    std::cout << res << std::endl;
    return 0;
}