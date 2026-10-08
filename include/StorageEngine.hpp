#pragma once
#include<vector>
#include <span>
#include <optional>



class StorageEngine{
public:
    virtual ~StorageEngine() = default;
    virtual std::optional<std::vector<std::byte>> get(std::span<const std::byte> key) = 0;
    virtual void set(std::span<const std::byte> key, std::span<const std::byte> value) = 0;
    virtual bool del(std::span<const std::byte> key) = 0;    
};