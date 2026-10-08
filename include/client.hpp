#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../include/kvprotocol.hpp"

class KVclient {
private:
    int fd;
    std::string serv_addr;
    uint16_t serv_port;

public:
    explicit KVclient(const std::string& addr, uint16_t port);
    ~KVclient();
    
    Response send_query(Query query);

private:
    void print_success(struct addrinfo* ai);
    int connect_server(const std::string& host, uint16_t port);
    Response read_response();
};