# kvstore

`kvstore` is a high-performance, event-driven key-value store implemented in C++. It features a non-blocking TCP server built with `poll()`, a custom hash table for data storage, and a simple command-line client for interaction.

## Features

-   **Client-Server Architecture**: A TCP-based server that can handle multiple client connections concurrently.
-   **Non-Blocking I/O**: Utilizes `poll()` for efficient, event-driven I/O, allowing the server to manage many connections with a single thread.
-   **Custom Hash Table**: A custom-built hash map that supports incremental resizing to avoid long pauses during rehashes.
-   **Custom Binary Protocol**: A lightweight binary protocol for efficient communication between the client and server.

## Project Structure

```
├── benchmarks/      # Scripts for performance testing
├── client/          # Client-side source code (CLI and connection logic)
├── docker/          # Dockerfiles and docker-compose.yml
├── include/         # Header files for the entire project
├── server/          # Server-side source code (main loop, connection handling)
└── src/             # Shared implementation files (hash table, protocol, helpers)
```

## Getting Started

### Prerequisites

-   A C++20 compatible compiler (e.g., GCC, Clang)
-   CMake (version 4.2 or later)
-   Docker and Docker Compose (for containerized deployment)

### Building with CMake

1.  Clone the repository:
    ```bash
    git clone https://github.com/jay2916/kvstore.git
    cd kvstore
    ```

2.  Create a build directory and run CMake:
    ```bash
    mkdir build
    cd build
    cmake ..
    ```

3.  Compile the project:
    ```bash
    make
    ```
    This will generate two executables in the `build` directory: `kvserver` and `kvclient`.

## Usage

### 1. Run the Server

Start the server and specify the port it should listen on.

```bash
# Using the binary built with CMake
./build/kvserver 4444
```

### 2. Run the Client

Connect the client to the server's IP address and port.

```bash
# Using the binary built with CMake
./build/kvclient 127.0.0.1 4444
```

### Client Commands

Once connected, you can use the following commands in the client's interactive prompt:

-   **SET**: Store a key-value pair.
    ```
    > set mykey myvalue
    Response{status: RES_OK ,data:  }
    ```

-   **GET**: Retrieve the value for a given key.
    ```
    > get mykey
    Response{status: RES_OK ,data: myvalue }
    ```

-   **DEL**: Delete a key-value pair.
    ```
    > del mykey
    Response{status: RES_OK ,data:  }
    ```

-   **EXIT**: Disconnect from the server and close the client.
    ```
    > exit
    ```
