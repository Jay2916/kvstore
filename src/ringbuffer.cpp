#include "../include/ringbuffer.hpp"

#include <algorithm>

template<typename T>
RingBuffer<T>::RingBuffer(size_t capacity)
    : buffer(capacity), capacity(capacity), size(0), read_pos(0), write_pos(0) {}

template<typename T>
size_t RingBuffer<T>::getcapacity() {
    return this->capacity;
}

template<typename T>
size_t RingBuffer<T>::readable() {
    return size;
}

template<typename T>
size_t RingBuffer<T>::writable() {
    return capacity - size;
}

template<typename T>
size_t RingBuffer<T>::consume(size_t n) {
    if (n <= size) {
        read_pos = (read_pos + n) % capacity;
        size -= n;
        return n;
    }

    return 0;
}

template<typename T>
std::vector<T> RingBuffer<T>::peek(size_t n) { //refactor: use std::copy 
    if (readable() < n) {
        return {};
    }
    std::vector<T> data;
    data.reserve(n);
    for (size_t i = 0; i < n; i++) {
        size_t pos = (read_pos + i) % capacity;
        data.push_back(buffer[pos]);
    }

    return data;
}

template<typename T>
size_t RingBuffer<T>::contigious_readable(){
    return std::min(size, this->capacity - read_pos);
}

template<typename T>
size_t RingBuffer<T>::contigious_writable(){
    if (size == capacity)
        return 0;
    if (write_pos >= read_pos)
        return capacity - write_pos;
    return read_pos - write_pos;
}
template<typename T>
T* RingBuffer<T>::front(){
    return &(this->buffer[read_pos]);
}
template<typename T>
size_t RingBuffer<T>::getsize(){
    return this->size;
}
template<typename T>
T* RingBuffer<T>::rear(){
    return &(this->buffer[write_pos]);
}
template<typename T>
T RingBuffer<T>::first(){
    return this->buffer[read_pos];
}
template<typename T>
bool RingBuffer<T>::empty(){
    return size == 0;
}
template<typename T>
size_t RingBuffer<T>::append(std::span<const T> data) {
    size_t dsize;
    if(writable() == 0) return 0;
    else dsize = std::min(data.size(), writable());

    if (write_pos + dsize <= capacity) {
        std::copy(
            data.begin(),
            data.end(),
            buffer.begin() + write_pos
        );
    } 
    else {
        size_t slice_pos = capacity - write_pos;

        std::copy(
            data.begin(),
            data.begin() + slice_pos,
            buffer.begin() + write_pos
        );

        std::copy(
            data.begin() + slice_pos,
            data.end(),
            buffer.begin()
        );
    }

    write_pos = (write_pos + dsize) % capacity;
    size += dsize;

    return dsize;
}
template class RingBuffer<std::byte>;