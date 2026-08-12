#include <span>
#include <cstdint>
#include <vector>

class RingBuffer{
private:
    std::vector<uint8_t> buffer;
    size_t size;
    size_t read_pos;
    size_t write_pos;
public:
    RingBuffer(int capacity);
    size_t capacity();
    size_t readable();
    size_t writable();
    size_t consume(size_t n);
    std::vector<uint8_t> peek(size_t n);
    size_t append(std::span<const uint8_t> data);

};