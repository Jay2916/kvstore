#pragma once
#include "../include/kvprotocol.hpp"
#include <span>
#include <vector>
#include <cstddef>

class Codec{
public:
    static std::vector<std::byte> encode_response(Response response);
    static Response decode_response(std::span<const std::byte> reader);
    static Query decode_query(std::span<const std::byte> reader);
    static std::vector<std::byte> encode_query(Query query);

};