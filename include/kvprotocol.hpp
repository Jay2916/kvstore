#pragma once
#include <string>
#include <iostream>
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
    std::string data;
};
const size_t MAXNSTR = 10;
const size_t MAX_RLEN = 1024;
const size_t MAX_QLEN = 1024;

std::ostream& operator<<(std::ostream& os, const Response& response);
std::string eval_status(Status i);