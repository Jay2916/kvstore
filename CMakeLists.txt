cmake_minimum_required(VERSION 4.2)

project(kvstore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(kvserver
    server/main.cpp
    src/hashtable.cpp
)

add_executable(kvclient
    client/main.cpp
)