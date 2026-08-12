#include <cstddef>
#include <vector>
#include <string>
#include <span>
#include <poll.h>
#include <cstdint>

#include "kvprotocol.hpp"
#include "hashtable.hpp"

struct Conn{
    int fd;
    bool want_read;
    bool want_write;
    bool want_close;
    std::vector<uint8_t> incoming;
    std::vector<uint8_t> outgoing;
};

struct Entry{
    HNode node;
    std::string key;
    std::string value;
};

class KVserver{
public:
    explicit KVserver(uint16_t const port);
    ~KVserver();

    void start();
    void stop();
private:

    struct {
        HMap db;
    } g_data;           //TODO: remove g_data wrapper

    std::vector<Conn*> fd2conn;
    std::vector<struct pollfd> poll_args;;
    uint16_t const PORT;
    int fd = -1;
    bool running = false;

    void start_listening();

    Conn* handle_accept(int fd);
    void handle_close(Conn *conn);
    void handle_read(Conn *conn);
    void handle_write(Conn *conn);

    void buf_append(std::vector<uint8_t> &dest_buf, const uint8_t* sr_buf,  size_t n);
    void buf_consume(std::vector<uint8_t> &buf, size_t n);

    bool try_one_request(Conn *conn);
    void do_request(Command cmd, std::vector<std::string> &input, std::vector<uint8_t> &out);
    int parse_request(std::span<const uint8_t> reader, Command &cmd, std::vector<std::string> &out);

    void do_get(std::string &key, Response &out);
    void do_del(std::string &key, Response &out);
    void do_set(std::string &key, std::string &value, Response &out);

    uint64_t str_hash(std::string str);
};