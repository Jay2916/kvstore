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

#kvstore_client -------------------------------------------------
add_library(kvstore_client STATIC
    src/client.cpp
    src/codec.cpp
    src/helper.cpp
    src/ringbuffer.cpp
)
target_compile_features(kvstore_client PUBLIC cxx_std_20)

target_compile_options(kvstore_client 
    PRIVATE -Wall -Wextra -Werror -g3)


#kvstore_server -------------------------------------------------
add_executable(kvstore_server
    server/main.cpp
    src/server.cpp
    src/codec.cpp
    src/helper.cpp
    src/ringbuffer.cpp
)
target_compile_options(kvstore_server 
    PRIVATE -Wall -Wextra -Werror -g3)

target_link_libraries(kvstore_server PRIVATE RequestDispatcher HashTableStorage RingBuffer Codec)


# Demo executable using the library ------------------------------
add_executable(kvclient_student_demo
    client/demo_main.cpp
)
add_executable(kvclient_cli_demo
    client/demo_cli.cpp
    client/cli.cpp
)

target_link_libraries(kvclient_student_demo PRIVATE kvstore_client)
target_link_libraries(kvclient_cli_demo PRIVATE kvstore_client)



