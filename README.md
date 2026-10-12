# kvstore

A C++20 in-memory key-value store with a Linux TCP server, a binary protocol, and a C++ client executable.

The server uses `epoll` to process connections and stores byte-vector keys and values in a custom hash table. The client executable runs a student-record demonstration.

## Contents

- [Features](#features)
- [Build requirements](#build-requirements)
- [Build and run](#build-and-run)
- [Client implementation](#client-implementation)
- [Storage and request handling](#storage-and-request-handling)
- [Binary protocol](#binary-protocol)

## Features

- TCP client/server communication.
- Linux `epoll` event loop with non-blocking accepted client sockets.
- `GET`, `SET`, and `DEL` operations.
- Binary keys and values represented by `std::vector<std::byte>`.
- Custom hash table with separate chaining and incremental rehashing.
- Ring buffers for connection input and output.
- In-memory storage.

## Build requirements

- CMake **3.25 or later**.
- A C++20 compiler and standard library.
- Linux for the server, which includes `<sys/epoll.h>`.

`CMakeLists.txt` requires C++20 and applies `-Wall`, `-Wextra`, `-Werror`, and `-g3` to both executable targets.

## Build and run

From the repository root on Linux:

```bash
cmake -S . -B build
cmake --build build
```

The executable targets produce `build/kvserver` and `build/kvclient`.

Start the server with a port argument:

```bash
./build/kvserver 4444
```

Run the client with the server address and port:

```bash
./build/kvclient 127.0.0.1 4444
```

The server binds to all IPv4 interfaces. Its entry point registers handlers for `SIGINT` and `SIGTERM` that call `stop()`.

## Client implementation

`include/client.hpp` declares `KVclient`, implemented in `client/client.cpp`.

| Method | Behavior |
| --- | --- |
| `KVclient(const std::string& addr, uint16_t port)` | Connects to the specified server |
| `send_query(Query query)` | Encodes and sends a query, then reads a response |
| `~KVclient()` | Closes the socket |


### Command loop

`client/cli.cpp` defines `cli_loop(KVclient&)` and is compiled into `kvclient`. The current `client/main.cpp` runs the student demonstration and does not call this loop.

| Parsed command | Behavior |
| --- | --- |
| `set <key> <value>` | Sends a `SET` query |
| `get <key>` | Sends a `GET` query |
| `del <key>` | Sends a `DEL` query |
| `exit` or `quit` | Exits the loop |

The loop parses keys and values as whitespace-separated tokens and prints responses.

## Storage and request handling

`KVserver` receives requests through connection ring buffers, decodes them with `Codec`, and passes them to `RequestDispatcher`. Responses are encoded and buffered for transmission.

`RequestDispatcher` operates through the `StorageEngine` interface. The server entry point supplies a `HashTableStorage` instance.

`HashTableStorage` uses FNV-1a hashing for byte-vector keys. Its hash map starts with four buckets, doubles when the load exceeds two entries per bucket, and migrates up to 64 entries per rehash helper invocation. Lookups check both tables while migration is in progress.

| Operation | Result |
| --- | --- |
| `GET` | Returns `RES_OK` and the value when present; otherwise `RES_NX` |
| `SET` | Inserts or overwrites a value and returns `RES_OK` |
| `DEL` | Returns `RES_OK` when deleted; otherwise `RES_NX` |

The dispatcher returns `RES_ERR` for missing required arguments or unsupported commands. It checks minimum argument counts.

## Binary protocol

`include/kvprotocol.hpp` defines `Query`, `Response`, `Command`, and `Status`. `src/codec.cpp` implements their encoding and decoding.

Length fields are unsigned 32-bit integers in big-endian byte order. Commands and statuses occupy one byte. The total length includes its own four-byte field.

### Request format

```text
total_length:u32 | command:u8 | argument_count:u32 |
argument_length:u32 | argument_bytes | ...
```

Each argument has its own length prefix.

| Command | Numeric code |
| --- | --- |
| `GET` | `0` |
| `SET` | `1` |
| `DEL` | `2` |

### Response format

```text
total_length:u32 | status:u8 | data_length:u32 | data_bytes
```

| Status | Numeric code |
| --- | --- |
| `RES_OK` | `0` |
| `RES_ERR` | `1` |
| `RES_NX` | `2` |

Successful `GET` responses contain the stored value. Successful `SET` and `DEL` responses contain empty data.

### Protocol constants

| Constant | Value |
| --- | --- |
| `MAXNSTR` | `2` |
| `MAX_QLEN` | `1024` |
| `MAX_RLEN` | `1024` |
| `MIN_SIZE` | `9` |

The encoders check message size excluding the four-byte total-length field. The decoders and server request-length check compare the total frame length against the corresponding limit.
