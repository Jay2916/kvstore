#include "../include/ringbuffer.hpp"


class RingBuffer{

    std::vector<uint8_t> buffer;
    size_t size;
    size_t read_pos;
    size_t write_pos;

public:
    RingBuffer(int capacity)
    :buffer(capacity), size(0), read_pos(0), write_pos(0){}

    size_t capacity(){
        return buffer.capacity();
    }
    size_t readable(){
        return size;
    }
    size_t writable(){
        return this->capacity() - size;
    }
    size_t consume(size_t n){
        if(n <= size){
            read_pos = (read_pos + n) % buffer.capacity();
            this->size -= n;
            return n;
        }
        else{
            return 0;
        }
    }
    std::vector<uint8_t> peek(size_t n){            //NEXT: implement spans somehow, accounting for wrapping > make it faster by avoiding copying into a new vector 
        if(this->readable() < n){
            return {};
        }
        std::vector<uint8_t> data{};
        data.reserve(n);
        size_t i = 0;
        size_t pos;
        while(i < n){
            pos = (read_pos + i) % this->capacity();
            data.push_back(buffer[pos]);
            i++;
        }
        return data;
    }
    size_t append(std::span<const uint8_t> data){
        size_t dsize = data.size();
        if(dsize > this->writable()){
            return 0;
        }
        if((write_pos + dsize) <= this->capacity()){
            std::copy(data.begin(), data.end(), buffer.begin() + write_pos);
        }
        else{
            size_t slice_pos = this->capacity() - write_pos; //number of positions left before wrapping around
            std::copy(data.begin(), data.begin() + slice_pos, buffer.begin() + write_pos);
            std::copy(data.begin() + slice_pos, data.end(), buffer.begin());
        }
        write_pos = (write_pos + dsize) % this->capacity();
        this->size += dsize;
        return dsize;

    }

};