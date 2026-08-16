#include <iostream>
#include <unistd.h>
#include <assert.h>
#include <span>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <cstddef>

#include "../include/helper.hpp"

int fd_set_nb(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return -1;

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        return -1;

    return 0;
}

void alert_msg(std::string msg){
    std::cout <<"Alert: "<< msg << std::endl;
}
void die(std::string msg) {
    int err = errno;
    std::cerr << "errno: " << err << ", " << msg << std::endl;
    abort();
}
void display_error(std::string msg){
    int err = errno;
    std::cerr << "errno: " << err << ", " << msg << std::endl;
}


int writeall(int fd, std::span<const std::byte> buf){
    ssize_t rsize;
    while(!buf.empty()){
        rsize = write(fd, buf.data(), buf.size());
        if(rsize < 0){
            if(errno == EINTR){
                continue;
            }
            return -1;
        }
        if(rsize == 0){
            return -1;
        }
        assert((size_t)rsize <= buf.size());
        buf = buf.subspan(static_cast<size_t>(rsize));
    }
    return 0;
}
int readfull(int fd, std::span<std::byte> buf) //make sure vector is large enough to accomodate any response
{
    while (!buf.empty()) {
        ssize_t rsize = read(fd, buf.data(), buf.size());
        if (rsize < 0){
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (rsize == 0) {
            alert_msg("server closed connection.");
            return -2;
        }
        buf = buf.subspan(static_cast<size_t>(rsize));
    }

    return 0;
}
void writeallwe(int fd, std::span<const std::byte> buf){
    int err = writeall(fd, buf);
    if(err){
        alert_msg("writeallwe() error");
    }
}
void readfullwe(int fd, std::span<std::byte> buf){
    int err = readfull(fd, buf);
    if (err == -2) {
        alert_msg("read(): EOF");
    } else if (err < 0) {
        alert_msg("readfullwe() error");
    }
}


void swrite_u32(std::span<std::byte> &out, uint32_t in){
    assert(out.size() >= 4);
    out[0] = static_cast<std::byte>(in >> 24);
    out[1] = static_cast<std::byte>(in >> 16);
    out[2] = static_cast<std::byte>(in >> 8);
    out[3] = static_cast<std::byte>(in);
    out = out.subspan(4);
}
void swrite_u8(std::span<std::byte> &out, uint8_t in){
    assert(out.size() >= 1);
    out[0] = static_cast<std::byte>(in);
    out = out.subspan(1);
}
void swrite_str(std::span<std::byte> &out, std::string_view in){
    assert(out.size() >= in.size() + 4);
    swrite_u32(out, static_cast<uint32_t>(in.size()));
    std::copy(
        reinterpret_cast<const std::byte*>(in.data()),
        reinterpret_cast<const std::byte*>(in.data()) + in.size(),
        out.begin()
    );
    out = out.subspan(in.size());
}
void swrite(std::span<std::byte> &out, const std::vector<std::byte> &in){
    size_t len = in.size();
    assert(out.size() >= len + 4);
    //add string len limit
    swrite_u32(out, static_cast<uint32_t>(len));
    std::copy(in.begin(), in.end(), out.begin());
    out = out.subspan(len);
}


uint32_t sread_u32(std::span<const std::byte>& in) {
    assert(in.size() >= 4);
    uint32_t out =
        (std::to_integer<uint32_t>(in[0]) << 24) |
        (std::to_integer<uint32_t>(in[1]) << 16) |
        (std::to_integer<uint32_t>(in[2]) << 8)  |
        std::to_integer<uint32_t>(in[3]);
    in = in.subspan(4);
    return out;
}
uint8_t sread_u8(std::span<const std::byte> &in){
    assert(in.size() >= 1);
    uint8_t out = std::to_integer<uint8_t>(in[0]);
    in = in.subspan(1);
    return out;
}
std::string sread_str(std::span<const std::byte> &in){
    uint32_t len = sread_u32(in);
    assert(in.size() >= len);
    std::string out(reinterpret_cast<const char*>(in.data()), len);
    in = in.subspan(len);
    return out;
}
std::vector<std::byte> sread(std::span<const std::byte> &in){
    uint32_t len = sread_u32(in);
    assert(in.size() >= len);
    std::vector<std::byte> out(in.begin(), in.begin() + len);
    in = in.subspan(len);
    return out;
}


template <typename T> static void print_vector(std::vector<T> vec){
    std::cout << "[ ";
    for(auto it = vec.begin(); it != vec.end(); it++){
        std::cout << *it << " "; 
    }
    std::cout << "] " << std::endl;
}

// uint32_t vread_u32(RingBuffer<std::byte>& rf){
//     assert(rf.getsize() >= 4);
//     std::vector<std::byte> in = rf.peek(4);
//     uint32_t out =  (static_cast<uint32_t>(in[0]) << 24) |
//                     (static_cast<uint32_t>(in[1]) << 16) |
//                     (static_cast<uint32_t>(in[2]) << 8)  |
//                     (static_cast<uint32_t>(in[3]));
//     rf.consume(4);
//     return out;
// }
uint32_t peek_u32(RingBuffer<std::byte>& rf){
    assert(rf.getsize() >= 4);
    std::vector<std::byte> in = rf.peek(4);
    uint32_t out =  (static_cast<uint32_t>(in[0]) << 24) |
                    (static_cast<uint32_t>(in[1]) << 16) |
                    (static_cast<uint32_t>(in[2]) << 8)  |
                    (static_cast<uint32_t>(in[3]));
    return out;
}
// std::string vread_str(RingBuffer<std::byte>& rf){
//     uint32_t len = vread_u32(rf);
//     assert(rf.getsize() >= len);
//     std::string out = std::string(reinterpret_cast<const char*>(rf.peek(len).data(), len));
//     rf.consume(len);
//     return out;
// }
// uint8_t vread_u8(RingBuffer<std::byte>& rf) {
//     assert(rf.getsize() >= 1);
//     uint8_t out = std::to_integer<uint8_t>(rf.first());
//     rf.consume(1);
//     return out;
// }
void print_bytes_as_chars(std::span<const std::byte> data) {
    for (std::byte b : data) {
        std::cout << static_cast<char>(std::to_integer<unsigned char>(b));
    }
    std::cout << '\n';
}