#include <span>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <optional>
#include "../include/hashtable.hpp"
#include "../include/StorageEngine.hpp"


struct Entry{
    HNode node;
    std::vector<std::byte> key;
    std::vector<std::byte> value;
};

//this class wraps around the c-like implementation of hashtable.cpp
class HashTableStorage : public StorageEngine{
public:
    std::optional<std::vector<std::byte>> get(std::span<const std::byte> key) override;
    void set(std::span<const std::byte> key, std::span<const std::byte>  value) override;
    bool del(std::span<const std::byte> key) override;
    void clear();
    size_t size();

private:
    HMap hmap;
    uint64_t hash_bytes(std::span<const std::byte> data);




};