#pragma once

#include <span>
#include <cstdint>
#include <vector>
#include <cstddef>

template<typename T>
class RingBuffer {
private:
    std::vector<T> buffer;
    size_t capacity;
    size_t size;
    size_t read_pos;
    size_t write_pos;

public:
    RingBuffer(size_t capacity);
    
    bool empty();
    size_t getcapacity();
    size_t readable();
    size_t writable();
    size_t consume(size_t n);
    std::vector<T> peek(size_t n);
    size_t append(std::span<const T> data);
    size_t contigious_readable();
    size_t contigious_writable();
    T* front();
    T* rear();
    T first();
    size_t getsize();

};

