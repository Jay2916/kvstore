#include <cstdint>
#include <vector>
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
const size_t MAX_RLEN = 1024;
const int MAX_QLEN = 1024; 

struct Response{
    Status status = Status::RES_ERR;
    std::string data;
};
static std::string eval_status(Status i){
    switch(i){
        case Status::RES_OK:    return "RES_OK";
        case Status::RES_ERR:   return "RES_ERR";
        case Status::RES_NX:    return "RES_NX";
        default:    return "INVALID";
    }
}
std::ostream& operator<<(std::ostream& os, const Response& response) {
    os << "Response{status: " << eval_status(response.status)
    << " ,data: " << std::string(response.data.begin(), response.data.end())
    << " }";
    return os;
}