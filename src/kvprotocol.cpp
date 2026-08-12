#include "../include/kvprotocol.hpp"

std::string eval_status(Status i){
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
