#pragma once

#include <cstddef>
#include <vector>
#include <string>
#include <span>
#include <poll.h>
#include <cstdint>
#include <sys/epoll.h>

#include "kvprotocol.hpp"
#include "hashtable.hpp"
#include "RequestDispatcher.hpp"

#define MAX_EVENTS 1024

struct Conn;
struct Entry;
class KVserver{
public:
    explicit KVserver(uint16_t const port, RequestDispatcher& rd);
    ~KVserver();

    void start();
    void stop();
private:

    uint16_t const PORT;
    RequestDispatcher& requestDispatcher;
    std::vector<struct epoll_event> events{MAX_EVENTS};
    int fd = -1;
    int epfd = -1;
    bool running = false;

    void start_listening();

    int handle_accept(int fd);
    void handle_close(Conn *conn);
    void handle_read(Conn *conn);
    void handle_write(Conn *conn);

    void buf_append(std::vector<uint8_t> &dest_buf, const uint8_t* sr_buf,  size_t n);
    void buf_consume(std::vector<uint8_t> &buf, size_t n);

    bool try_one_request(Conn *conn);
    int parse_request(std::span<const std::byte> reader, Command &cmd, std::vector<std::vector<std::byte>> &out);
    int do_request(Command cmd, std::vector<std::vector<std::byte>> &input, Conn* conn);

    void do_get(std::vector<std::byte> &key, Response &out);
    void do_del(std::vector<std::byte> &key, Response &out);
    void do_set(std::vector<std::byte> &key, std::vector<std::byte> &value, Response &out);

    uint64_t hash_bytes(std::span<const std::byte> data);
    void update_epoll_event(Conn* conn);
};
