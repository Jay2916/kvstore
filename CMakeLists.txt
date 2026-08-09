cmake_minimum_required(VERSION 4.2)

project(kvstore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(kvserver
    server/main.cpp
    src/hashtable.cpp
    src/helper.cpp
)

add_executable(kvclient
    client/new_client.cpp
    src/helper.cpp
)

target_compile_options(kvclient 
    PRIVATE -Wall -Wextra -Werror)