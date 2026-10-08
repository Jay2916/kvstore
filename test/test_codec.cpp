#include <iostream>
#include<vector>
#include <cstddef>
#include <span>
#include <string>
#include "../include/kvprotocol.hpp"
#include "../include/codec.hpp"
std::string toString(const std::vector<std::byte>& bytes) {
    return std::string(
        reinterpret_cast<const char*>(bytes.data()),
        bytes.size()
    );
}
std::vector<std::byte> toBytes(std::string_view str) {
    const auto* ptr =
        reinterpret_cast<const std::byte*>(str.data());

    return {ptr, ptr + str.size()};
}
using namespace std;
void test_query(){
    cout << "Testing Queries: \n" << endl;
    std::vector<Query> testQueries = {
    {Command::GET, {}},
    {Command::GET, {{}}},
    {Command::GET, {{std::byte{0x00}}}},
    {Command::GET, {{std::byte{0xFF}}}},
    {Command::GET, {{std::byte{0x01}, std::byte{0x02}}}},

    {Command::SET, {}},
    {Command::SET, {{}}},
    {Command::SET, {{std::byte{0x01}}}},
    {Command::SET, {{std::byte{0x01}}, {}}},
    {Command::SET, {{std::byte{0x01}}, {std::byte{0xFF}}}},

    {Command::DEL, {}},
    {Command::DEL, {{}}},
    {Command::DEL, {{std::byte{0x00}}}},
    {Command::DEL, {{std::byte{0xAA}, std::byte{0x55}}}},

    {Command::GET, {
        {std::byte{0x00}, std::byte{0xFF}},
        {std::byte{0xAA}, std::byte{0x55}}
    }},

    {Command::SET, {
        std::vector<std::byte>(MAX_QLEN / 2, std::byte{0xAA}),
        std::vector<std::byte>((MAX_QLEN / 2), std::byte{0xBB})
    }},                                                                     //should fail

    {Command::DEL, {{std::byte{0x00}}}},
    {Command::DEL, {std::vector<std::byte>(MAX_QLEN + 1, std::byte{0xFF})}} //should fail

    };
    int i = 1;
    for(const auto& query : testQueries){
        std::vector<std::byte> enc_query = Codec::encode_query(query);
        Query new_query = Codec::decode_query(enc_query);
        if(new_query == query){
            cout << i << ": success" << endl;
        }
        else{
            cout << i << ": fail" << endl;
        }
        i++;
    }
    cout << "All test done.\n" << endl;
}
void test_response(){
    cout << "Testing responses: \n" << endl;
    std::vector<Response> testResponses = {
    {Status::RES_OK,  {}},
    {Status::RES_ERR, {}},
    {Status::RES_NX,  {}},

    {Status::RES_OK,  {std::byte{0x00}}},
    {Status::RES_ERR, {std::byte{0xFF}}},
    {Status::RES_NX,  {std::byte{0x80}}},

    {Status::RES_OK,  {std::byte{0x00}, std::byte{0xFF}}},
    {Status::RES_ERR, {std::byte{0xAA}, std::byte{0x55}}},
    {Status::RES_NX,  {std::byte{0x01}, std::byte{0x02},
                       std::byte{0x03}, std::byte{0x04}}},

    {Status::RES_OK,
        std::vector<std::byte>(MAX_RLEN - MIN_SIZE + 1, std::byte{0xAA})}, //should fail

    {Status::RES_ERR,
        std::vector<std::byte>(MAX_RLEN - MIN_SIZE, std::byte{0xFF})}
    };
    int i = 1;
    for (const auto& expected : testResponses) {
        auto encoded = Codec::encode_response(expected);
        auto decoded = Codec::decode_response(encoded);
        if(decoded == expected){
            cout << i << ": success" << endl;
        }
        else{
            cout << i << ": fail" << endl;
        }
        i++;
    }
    cout << "All tests done.\n" << endl;
}

int main(){
    test_query();
    test_response();
    return 0;
}