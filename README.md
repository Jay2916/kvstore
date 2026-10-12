# kvstore

A C++20 in-memory key-value store with a Linux TCP server, a binary protocol, and a reusable C++ client.

`kvstore` explores the core components of a networked storage engine: socket I/O, request framing, command dispatch, and a custom hash table with incremental rehashing. Keys and values are byte sequences, so applications can store text or serialize their own structured data.

> **Current behavior:** the server uses Linux `epoll`. The `kvclient` executable runs a hardcoded student-record demonstration; an interactive command loop exists in the source but is not called by the current executable.

## Contents

- [Features](#features)
- [Requirements](#requirements)
- [Build and run](#build-and-run)
- [Reusable client library](#reusable-client-library)
- [Client API](#client-api)
- [Interactive CLI](#interactive-cli)
- [Architecture](#architecture)
- [Binary protocol](#binary-protocol)

## Features

- **TCP client/server:** accepts multiple client connections in a single event loop.
- **Event-driven I/O:** uses `epoll` and non-blocking accepted client sockets.
- **Three operations:** `GET`, `SET`, and `DEL`.
- **Binary keys and values:** uses `std::vector<std::byte>` and `std::span`.
- **Custom storage:** separate-chaining hash table with FNV-1a hashing for keys.
- **Incremental rehashing:** migrates up to 64 entries per rehash helper invocation rather than moving all entries at once.
- **Buffered connections:** ring buffers and pending-response state handle incoming and outgoing data.
- **Separated components:** transport, codec, dispatcher, and storage have distinct interfaces.

Data is held in memory and is lost when the server exits. Persistence, replication, authentication, and TLS are not implemented.

## Requirements

For a native server build:

- Linux, because the server includes `<sys/epoll.h>`.
- A C++20 compiler and standard library, such as GCC or Clang with C++20 support.
- CMake **3.25 or later**, as specified in `CMakeLists.txt`.
- A build tool supported by CMake, such as Make or Ninja.

Docker is optional for running the Linux server on another operating system. Python 3 is used by the benchmark scripts.

## Build and run

Run these commands from the repository root on Linux:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
```

The build produces:

| Executable | Purpose |
| --- | --- |
| `build/kvserver` | TCP server backed by in-memory hash-table storage |
| `build/kvclient` | Client demonstration using a serialized student record |

Start the server in one terminal:

```bash
./build/kvserver 4444
```

Run the client in another terminal:

```bash
./build/kvclient 127.0.0.1 4444
```

The client uses key `1234` and performs this sequence:

1. Stores a student with roll number `2`, name `jaychirmade`, and marks `2`, `300`, and `4`.
2. Retrieves and prints the student.
3. Deletes the key.
4. Retrieves it again, receiving `RES_NX` because it no longer exists.

Successful operation statuses are `RES_OK`, `RES_OK`, `RES_OK`, and `RES_NX`. The client also prints connection and I/O diagnostics.

Use `Ctrl+C` to stop the server. The server listens on all IPv4 interfaces (`0.0.0.0`); use an isolated development environment or restrict network access when running it.

### Build only the client

The client uses POSIX networking and can be built separately on a compatible Unix-like system without compiling the Linux server:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target kvclient
```

It still needs a running server to perform requests.

## Reusable client library

The supplied project builds a client executable. To reuse its connection and protocol code through a CMake library target, make the following changes once in the kvstore repository.

### Define the library

Replace the existing `add_executable(kvclient ...)` and `target_link_libraries(kvclient ...)` blocks in the root `CMakeLists.txt` with:

```cmake
add_library(kvstore_client STATIC
    client/client.cpp
    src/codec.cpp
    src/helper.cpp
    src/ringbuffer.cpp
)

add_library(kvstore::client ALIAS kvstore_client)

target_compile_features(kvstore_client PUBLIC cxx_std_20)

target_include_directories(kvstore_client PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

add_executable(kvclient
    client/main.cpp
    client/cli.cpp
)

target_link_libraries(kvclient PRIVATE kvstore::client)
```

Keep the existing server targets and compiler settings. `main.cpp` and `cli.cpp` remain part of the demo executable; the library contains the transport, codec, and shared helpers. Do not also compile these library implementation files directly into `kvclient`.

The `PUBLIC` settings automatically provide the include directory and C++20 requirement to applications linking `kvstore::client`.

### Prevent socket ownership from being copied

`KVclient` owns a socket and closes it in its destructor. Add these declarations in the `public` section of `include/client.hpp` to prevent two copied clients from owning the same socket:

```cpp
KVclient(const KVclient&) = delete;
KVclient& operator=(const KVclient&) = delete;
```

### Build the library and demo

From the repository root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target kvstore_client kvclient
```

This produces `build/libkvstore_client.a` and `build/kvclient` on compatible Unix-like systems. The library can be built independently with `--target kvstore_client`. A running server is required only when sending requests.

### Use the library in another project

Place the updated kvstore sources inside your application's `vendor/kvstore` directory:

```text
my-app/
├── CMakeLists.txt
├── main.cpp
└── vendor/
    └── kvstore/
        ├── CMakeLists.txt
        ├── include/
        ├── client/
        ├── src/
        └── server/
```

Use this `CMakeLists.txt` in `my-app`:

```cmake
cmake_minimum_required(VERSION 3.25)
project(my_app LANGUAGES CXX)

add_subdirectory(vendor/kvstore EXCLUDE_FROM_ALL)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE kvstore::client)
```

`EXCLUDE_FROM_ALL` keeps unrelated kvstore targets, including the server, out of the application's default build. The client library and its required sources are built because `my_app` links them.

Save the [Client API example](#client-api) below as `my-app/main.cpp`. With a server listening on port `4444`, build and run the application from `my-app`:

```bash
cmake -S . -B build
cmake --build build --target my_app
./build/my_app
```

Applications include `client.hpp`, `kvprotocol.hpp`, and any required helper headers directly; they do not need to compile the client sources themselves. This setup reuses the library from source through CMake. An installed package supporting `find_package(kvstore)` requires additional install and export configuration.

## Client API

`KVclient` connects to a host and port on construction. Call `send_query(Query)` to send an operation and receive a `Response`.

```cpp
#include "client.hpp"
#include "helper.hpp"
#include "kvprotocol.hpp"
#include <iostream>

int main() {
    KVclient client{"127.0.0.1", 4444};

    Response stored = client.send_query(
        Query{Command::SET, {toBytes("username"), toBytes("jay")}}
    );
    if (stored.status != Status::RES_OK) {
        return 1;
    }

    Response fetched = client.send_query(
        Query{Command::GET, {toBytes("username")}}
    );
    if (fetched.status == Status::RES_OK) {
        std::cout << toString(fetched.data) << '\n';
    }

    Response deleted = client.send_query(
        Query{Command::DEL, {toBytes("username")}}
    );
    return deleted.status == Status::RES_OK ? 0 : 1;
}
```

Use this as `main.cpp` in the application setup above. Alternatively, without making the library changes, save it as `example.cpp` in the kvstore repository root and compile the required sources directly:

```bash
c++ -std=c++20 -Iinclude example.cpp client/client.cpp \
    src/codec.cpp src/helper.cpp src/ringbuffer.cpp -o example
./example
```

`toBytes` and `toString` are convenient for text. For binary application data, supply byte vectors and decode the returned bytes according to your application's format. Check the response status before decoding a value.

### Command semantics

| Command | Arguments | Result |
| --- | --- | --- |
| `GET` | Key | `RES_OK` with the value, or `RES_NX` when absent |
| `SET` | Key, value | Inserts or overwrites the value; returns `RES_OK` |
| `DEL` | Key | `RES_OK` when deleted, or `RES_NX` when absent |

Missing required arguments and unsupported command codes produce `RES_ERR` at the dispatcher level. The dispatcher checks minimum argument counts; it does not strictly reject every extra argument. The codec allows at most two arguments.

## Interactive CLI

`client/cli.cpp` implements `cli_loop(KVclient&)`, but `client/main.cpp` currently runs the student demo instead. To enable the prompt, replace the demonstration's operations with this call after constructing the client:

```cpp
cli_loop(client);
```

Then rebuild `kvclient`. The loop supports lowercase commands:

```text
set mykey myvalue
get mykey
del mykey
exit
```

`quit` also exits. Keys and values are parsed as whitespace-separated tokens; quoted values and values containing spaces are not supported by this parser.

## Architecture

```text
Application / optional CLI
          |
       KVclient
          |
    Codec -> TCP -> epoll event loop
                         |
                 Connection ring buffers
                         |
                       Codec
                         |
                 RequestDispatcher
                         |
                    StorageEngine
                         |
                  HashTableStorage
                         |
                 Custom HMap / HTab
```

The server assembles complete request frames from connection buffers, decodes them, dispatches operations to storage, and encodes responses. The client uses synchronous request/response I/O.

`StorageEngine` defines the storage interface. `HashTableStorage` implements it using byte-vector entries and a custom hash map. The map starts with four buckets, doubles when its load exceeds two entries per bucket, and searches both old and new tables while rehashing is in progress.

## Binary protocol

This is a custom protocol; it is not Redis RESP or an HTTP API. All frame length and argument length fields are unsigned 32-bit integers in **big-endian** byte order. Command and status codes occupy one byte.

### Request frame

```text
total_length:u32 | command:u8 | argument_count:u32 |
argument_length:u32 | argument_bytes | ...
```

`total_length` includes its own four bytes. Each argument has its own length prefix, and keys/values are arbitrary bytes.

| Command | Numeric code | Normal argument count |
| --- | --- | --- |
| `GET` | `0` | `1` |
| `SET` | `1` | `2` |
| `DEL` | `2` | `1` |

For example, `GET` with the one-byte key `k` encodes as:

```text
00 00 00 0e  00  00 00 00 01  00 00 00 01  6b
```

### Response frame

```text
total_length:u32 | status:u8 | data_length:u32 | data_bytes
```

| Status | Numeric code | Meaning |
| --- | --- | --- |
| `RES_OK` | `0` | Operation succeeded |
| `RES_ERR` | `1` | Operation error |
| `RES_NX` | `2` | Key does not exist |

Successful `GET` returns the stored bytes. Successful `SET` and `DEL` return empty data.
