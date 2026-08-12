#include <iostream>
#include <unistd.h>
#include <assert.h>
#include <span>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>

#include "../include/helper.hpp"

void fd_set_nb(int fd) {
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

void alert_msg(std::string msg){
    std::cout <<"Alert: "<< msg << std::endl;
}
void die(std::string msg) {
    int err = errno;
    std::cerr << "errno: " << err << ", " << msg << std::endl;
    abort();
}


int writeall(int fd, std::span<const uint8_t> buf){
    ssize_t rsize;
    while(!buf.empty()){
        rsize = write(fd, buf.data(), buf.size());
        if(rsize <= 0){
            return -1;
        }
        assert((size_t)rsize <= buf.size());
        buf = buf.subspan(static_cast<size_t>(rsize));
    }
    return 0;
}
int readfull(int fd, std::span<uint8_t> buf) //make sure vector is large enough to accomodate any response
{
    while (!buf.empty()) {
        ssize_t rsize = read(fd, buf.data(), buf.size());
        if (rsize < 0)
            return -1;
        if (rsize == 0) {
            alert_msg("server closed connection.");
            return -2;
        }
        buf = buf.subspan(static_cast<size_t>(rsize));
    }

    return 0;
}
void writeallwe(int fd, std::span<const uint8_t> buf){
    int err = writeall(fd, buf);
    if(err){
        alert_msg("write() error");
    }
}
void readfullwe(int fd, std::span<uint8_t> buf){
    int err = readfull(fd, buf);
    if(err){
        alert_msg((errno == 0)? "read(): EOF":"read() error");
    }
}


void vwrite_u32(std::span<uint8_t> &out, uint32_t in){
    assert(out.size() >= 4);
    out[0] = static_cast<uint8_t>(in >> 24);
    out[1] = static_cast<uint8_t>(in >> 16);
    out[2] = static_cast<uint8_t>(in >> 8);
    out[3] = static_cast<uint8_t>(in);
    out = out.subspan(4);
}
void vwrite_u8(std::span<uint8_t> &out, uint8_t in){
    assert(out.size() >= 1);
    out[0] = in;
    out = out.subspan(1);
}
void vwrite_str(std::span<uint8_t> &out, std::string_view in){
    assert(out.size() >= in.size() + 4);
    vwrite_u32(out, static_cast<uint32_t>(in.size()));
    std::copy(in.begin(), in.end(), out.begin());
    out = out.subspan(in.size());
}


uint32_t vread_u32(std::span<const uint8_t> &in){
    assert(in.size() >= 4);
    uint32_t out =  (static_cast<uint32_t>(in[0]) << 24) |
                    (static_cast<uint32_t>(in[1]) << 16) |
                    (static_cast<uint32_t>(in[2]) << 8)  |
                    (static_cast<uint32_t>(in[3]));
    in = in.subspan(4);
    return out;
}
uint8_t vread_u8(std::span<const uint8_t> &in){
    assert(in.size() >= 1);
    uint8_t out = in[0];
    in = in.subspan(1);
    return out;
}
std::string vread_str(std::span<const uint8_t> &in){
    uint32_t len = vread_u32(in);
    assert(in.size() >= len);
    std::string out = std::string(in.begin(), in.begin() + len);
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