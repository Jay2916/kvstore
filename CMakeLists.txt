cmake_minimum_required(VERSION 3.25)

project(kvstore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(kvserver
    server/main.cpp
    server/server.cpp
    src/hashtable.cpp
    src/helper.cpp
    src/kvprotocol.cpp
    src/ringbuffer.cpp
)

add_executable(kvclient
    client/client.cpp
    client/cli.cpp
    client/main.cpp
    src/helper.cpp
    src/kvprotocol.cpp
    src/ringbuffer.cpp
    
)

target_compile_options(kvclient 
    PRIVATE -Wall -Wextra -Werror)