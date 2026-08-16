#include <iostream>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <span>

#include "../include/helper.hpp"
#include "../include/cli.hpp"
#include "../include/client.hpp"

KVclient::KVclient(const std::string &addr, uint16_t port)
    :fd(-1), serv_addr(addr), serv_port(port){
    int rv = this->connect_server(serv_addr, serv_port);
    if(rv == -1){
        std::cout << addr << ":" << port << std::endl;
        die("connect_server failure");
    }
}
KVclient::~KVclient(){
    alert_msg("Disconnected from server.");
    close(fd);
}

void KVclient::print_success(struct addrinfo* ai){
    char host[NI_MAXHOST];
    char serv[NI_MAXSERV];
    int err = getnameinfo(ai->ai_addr, ai->ai_addrlen, host, sizeof(host), serv, sizeof(serv), NI_NUMERICHOST | NI_NUMERICSERV);
    if(err == 0)    std::cout << "Connected to " << host << ":" << serv << std::endl;
    else            alert_msg("getnameinfo error");
}
int KVclient::connect_server(const std::string &host, const uint16_t port) {
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

Response KVclient::send_query(Command cmd, std::vector<std::string> input){
    std::vector<std::byte> out(MAX_QLEN);
    uint32_t tlen = 1 + 4;
    std::span<std::byte> writer = out;
    writer = writer.subspan(4); //reserve space for total len
    swrite_u8(writer, static_cast<uint8_t>(cmd));
    swrite_u32(writer, static_cast<uint32_t>(input.size()));
    for(const auto& it : input){
        swrite_str(writer, it);
        tlen += 4 + it.size();
    }
    writer = out;
    swrite_u32(writer, tlen);
    print_bytes_as_chars(
        std::span<const std::byte>(out).first(tlen + 4)
    );
    writeallwe(fd, std::span(out).first(tlen + 4));
#ifndef NO_DEBUG
    std::cout << "write: " << tlen + 4 << std::endl;
#endif
    return read_response();
}

Response KVclient::read_response(){
    std::byte len_buf[4];
    readfullwe(fd, len_buf);
    std::span<const std::byte> len_reader = len_buf;
    size_t tlen = (sread_u32(len_reader));
    if(tlen > MAX_RLEN){
        alert_msg("Response data too long.");
        return {};
    }
    std::vector<std::byte> in(tlen);
    readfullwe(fd, in);
    std::span<const std::byte> reader = in;
    Response res;
    res.status = static_cast<Status>(sread_u8(reader));
    res.data = sread(reader);
    assert(reader.empty()); //trailing garbage
#ifndef NO_DEBUG
    std::cout << "read: " << tlen + 4<< std::endl;
#endif
    return res;
}

    


