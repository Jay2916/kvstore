#pragma once
#include <string>
#include <iostream>
#include <cstdint>
#include <vector>

/*
Response : {total_len | status | data_len | data }
Query : {total_len | Command | nstr | str_i_len | str_i | ...}
*/


#define MIN_SIZE (4 + 1 + 4)

enum class Command: uint8_t{
    GET, 
    SET,
    DEL
};
enum class Status: uint8_t{
    RES_OK,
    RES_ERR,
    RES_NX
};

struct Response{
    Status status;
    std::vector<std::byte> data;
    bool operator==(const Response&) const = default;
};

struct Query {
    Command cmd;
    std::vector<std::vector<std::byte>> args;
    bool operator==(const Query&) const = default;
};

const size_t MAXNSTR = 2;
const size_t MAX_RLEN = 1024;
const size_t MAX_QLEN = 1024;


