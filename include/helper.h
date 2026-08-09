#pragma once
#include <vector>
#include <string>
#include <span>
#include <cstdint>

template <typename T> static void print_vector(std::vector<T> vec);


void die(std::string msg);
void alert_msg(std::string msg);

int readfull(int fd, std::span<uint8_t> buf);
int writeall(int fd, std::span<const uint8_t> buf);
void readfullwe(int fd, std::span<uint8_t> buf);
void writeallwe(int fd, std::span<const uint8_t> buf);

void vwrite_u8(std::span<uint8_t> &out, uint8_t in);
void vwrite_u32(std::span<uint8_t> &out, uint32_t in);
void vwrite_str(std::span<uint8_t> &out, std::string_view in);

uint8_t vread_u8(std::span<const uint8_t> &in);
uint32_t vread_u32(std::span<const uint8_t> &in);
std::string vread_str(std::span<const uint8_t> &in);