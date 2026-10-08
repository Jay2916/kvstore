#include "../include/codec.hpp"
#include "../include/helper.hpp"
#include <assert.h>
/*
network serialization format,
Response : {total_len | status | data_len | data }
Query : {total_len | Command | nstr | str_i_len | str_i | ...}

total_len includes bytes occupied by the total_len itself i.e. 4bytes
*/

//TODO: assert is temporary, replace with alert msg or something

std::vector<std::byte> Codec::encode_response(Response response){
    uint32_t tlen = MIN_SIZE + response.data.size();
    std::vector<std::byte> enc_response;
    enc_response.resize(tlen);
    std::span<std::byte> writer = enc_response;
    swrite_u32(writer, tlen);
    swrite_u8(writer, static_cast<uint8_t>(response.status));
    swrite(writer, response.data);
    assert(writer.empty());
    if(enc_response.size() - 4 > MAX_RLEN){ //total len not accounted in max length
        alert_msg("encode_response: Response too long");
        return {};
    }
    return enc_response;
}
std::vector<std::byte> Codec::encode_query(Query query){
    std::vector<std::byte> enc_query;
    uint32_t tlen = MIN_SIZE;
    for(const auto& arg : query.args){
        tlen += 4 + arg.size();
    }
    enc_query.resize(tlen);
    std::span<std::byte> writer = enc_query;
    swrite_u32(writer, tlen);
    swrite_u8(writer, static_cast<uint8_t>(query.cmd));
    swrite_u32(writer, static_cast<uint32_t>(query.args.size()));
    for(const auto& arg : query.args){
        swrite(writer, arg);
    }
    assert(writer.empty());
    if(enc_query.size() - 4 > MAX_QLEN){ //total len not accounted in max length
        alert_msg("encode_query: Query too long");
        return {};
    }
    return enc_query;
}
Response Codec::decode_response(std::span<const std::byte> reader){
    Response response = {};
    if(reader.size() < MIN_SIZE){
        alert_msg("decode_response: Response too small"); //Response is smaller than min possible i.e. corrupted data
        return {};
    }
    uint32_t tlen = sread_u32(reader);
    if(tlen > MAX_RLEN){
        alert_msg("decode_response: Response too long");
        return {};
    }
    assert(reader.size() + 4 == static_cast<size_t>(tlen));
    response.status = static_cast<Status>(sread_u8(reader));
    switch(response.status){
        case Status::RES_OK:
        case Status::RES_NX:
        case Status::RES_ERR:
            break;
        default:
            alert_msg("decode_response: invalid status");
            return {}; //invalid status
    }
    response.data = sread(reader);
    assert(reader.empty());
    return response;
}

Query Codec::decode_query(std::span<const std::byte> reader){
    Query query = {};
    if(reader.size() < MIN_SIZE){
        alert_msg("decode_query: Query too small"); //query is smaller than min possible i.e. corrupted data
        return {};
    }
    uint32_t tlen = sread_u32(reader);
    if(tlen > MAX_QLEN){
        alert_msg("decode_query: Query too long");
        return {};
    }
    assert(reader.size() + 4 == static_cast<size_t>(tlen));
    query.cmd = static_cast<Command>(sread_u8(reader));
    switch (query.cmd)
    {
    case Command::GET:
    case Command::SET:
    case Command::DEL:
        break;
    
    default:
        alert_msg("decode_query: invalid command");
        return {};
    }
    uint32_t n = sread_u32(reader);
    if(n > MAXNSTR){
        return {};      //protocol error
    }
    while(query.args.size() < (size_t)n){
        query.args.push_back(sread(reader));
    }
    assert(reader.empty());
    return query;
}



