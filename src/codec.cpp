#include "../include/codec.hpp"

std::vector<std::byte> Codec::encode_response(Response response){
    uint32_t tlen = HEADER_LEN + 4 + response.data.size()
    std::vector<std::byte> enc_response;
    enc.reserve(tlen);
    std::span<std::byte> writer = enc_response;
    swrite_u32(writer, tlen);
    swrite_u8(writer, static_cast<uint8_t>response.status);
    swrite(writer, response.data);
    return enc_response;
}
bool Codec::decode_query(std::span<const std::byte> reader, Query &query){
    query.cmd = static_cast<Command>(sread_u8(reader));
    uint32_t n = sread_u32(reader);
    if(n > MAXNSTR){
        return false;      //protocol error
    }
    while(out.size() < (size_t)n){
        query.args.push_back(sread(reader));
    }
    if (!reader.empty())
        return false;
    return true;
}



