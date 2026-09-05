#include "../include/kvprotocol.hpp"
#include <span>
class Codec{
public:
    std::vector<std::byte> encode_response(Response response);
    decode_response();
    encode_query();
    bool decode_query(std::span<std::byte> reader, Query &query);

private:

}