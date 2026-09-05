#pragma once
#include <string>
#include <iostream>
#include <cstdint>
#include <vector>

#define HEADER_LEN 5
enum class Command: uint8_t{
    GET, 
    SET,
    DEL,
    INV
};
enum class Status: uint8_t{
    RES_OK,
    RES_ERR,
    RES_NX
};

struct Response{
    Status status;
    std::vector<std::byte> data;
};

struct Query{
    Command cmd;
    std::vector<std::vector<std::byte>> args;
};
const size_t MAXNSTR = 10;
const size_t MAX_RLEN = 1024;
const size_t MAX_QLEN = 1024;

std::ostream& operator<<(std::ostream& os, const Response& response);
std::string eval_status(Status i);