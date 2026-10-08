cmake_minimum_required(VERSION 3.25)

project(kvstore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(HashTableStorage
    src/HashTableStorage.cpp
    src/hashtable.cpp
)

add_library(RequestDispatcher
    src/RequestDispatcher.cpp 
    src/helper.cpp
)

add_library(RingBuffer
    src/ringbuffer.cpp 
)

add_library(Codec
    src/codec.cpp 
    src/helper.cpp
)

add_executable(kvserver
    server/main.cpp
    server/server.cpp
    src/codec.cpp
    src/helper.cpp
    src/ringbuffer.cpp
)
target_link_libraries(kvserver PRIVATE RequestDispatcher HashTableStorage RingBuffer Codec)

add_executable(kvclient
    client/client.cpp
    client/cli.cpp
    client/main.cpp
    src/helper.cpp
    
)
target_link_libraries(kvclient PRIVATE RingBuffer Codec)

target_compile_options(kvclient 
    PRIVATE -Wall -Wextra -Werror -g3)

target_compile_options(kvserver 
    PRIVATE -Wall -Wextra -Werror -g3)