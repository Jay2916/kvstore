#pragma once
#include <vector>
#include <string>
#include <span>
#include <cstdint>

#include "../include/kvprotocol.hpp"
#include "../include/ringbuffer.hpp"



int fd_set_nb(int fd);

void die(std::string msg);
void alert_msg(std::string msg);
void die(std::string msg);
void display_error(std::string msg);

//these are used only by client
int readfull(int fd, std::span<std::byte> buf);
int writeall(int fd, std::span<const std::byte> buf);
void readfullwe(int fd, std::span<std::byte> buf);
void writeallwe(int fd, std::span<const std::byte> buf);
//-------------------------------------------------------

void swrite_u8(std::span<std::byte> &out, uint8_t in);
void swrite_u32(std::span<std::byte> &out, uint32_t in);
void swrite_str(std::span<std::byte> &out, std::string_view in);
void swrite(std::span<std::byte> &out, const std::vector<std::byte> &in);

uint8_t sread_u8(std::span<const std::byte> &in);
uint32_t sread_u32(std::span<const std::byte> &in);
std::string sread_str(std::span<const std::byte> &in);
std::vector<std::byte> sread(std::span<const std::byte> &in);


// uint8_t vread_u8(RingBuffer<std::byte>& rf);
// uint32_t vread_u32(RingBuffer<std::byte>& rf);
// std::string vread_str(RingBuffer<std::byte>& rf);
uint32_t peek_u32(RingBuffer<std::byte>& rf);

template <typename T> static void print_vector(std::vector<T> vec);

void print_bytes_as_chars(std::span<const std::byte> data);