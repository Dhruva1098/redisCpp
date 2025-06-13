# Redis-like Key-Value Store in C++

A lightweight, in-memory key-value store implementation inspired by Redis, written in C++. This project demonstrates core concepts of networking, data structures, and system programming.

## Features

- In-memory key-value storage with custom hashmap implementation
- TCP server with non-blocking I/O
- Event-driven architecture using `poll()`
- Binary protocol for efficient communication
- Basic Redis-like commands (GET, SET, DEL)
- Progressive hashmap resizing for better performance

## Architecture

### Components

1. **Server (`server_end.cpp`)**
   - TCP server implementation
   - Non-blocking I/O with `poll()`
   - Connection state management
   - Request handling and response generation

2. **Client (`client_end.cpp`)**
   - Simple TCP client
   - Command-line interface
   - Binary protocol implementation

3. **Custom Hashmap (`hashtab.h`, `hashmap.cpp`)**
   - Custom hash table implementation
   - Progressive resizing
   - Open addressing with chaining
   - Efficient collision handling

### Data Structures

1. **Hashmap Implementation**
   ```cpp
   struct HNode {
       HNode *next = NULL;
       uint64_t hcode = 0;
   };

   struct HTab {
       HNode **tab = NULL;
       size_t mask = 0;
       size_t size = 0;
   };

   struct HMap {
       HTab ht1;  // newer
       HTab ht2;  // older
       size_t resizing_pos = 0;
   };
   ```

2. **Key-Value Entry**
   ```cpp
   struct Entry {
       HNode node;
       std::string key;
       std::string val;
   };
   ```

### Protocol

The server uses a binary protocol for communication:

1. **Request Format**
   - 4 bytes: total message length
   - 4 bytes: number of arguments
   - For each argument:
     - 4 bytes: argument length
     - N bytes: argument data

2. **Response Format**
   - 4 bytes: response code
   - N bytes: response data

## Building

### Prerequisites
- C++11 compatible compiler
- Make (optional)

### Compilation

1. Compile the server:
   ```bash
   g++ -o server server_end.cpp hashmap.cpp -std=c++11
   ```

2. Compile the client:
   ```bash
   g++ -o client client_end.cpp -std=c++11
   ```

## Usage

1. Start the server:
   ```bash
   ./server
   ```

2. Use the client to interact with the server:
   ```bash
   # Set a key-value pair
   ./client set mykey myvalue

   # Get a value
   ./client get mykey

   # Delete a key
   ./client del mykey
   ```

## Implementation Details

### Server Architecture

1. **Connection Handling**
   - Non-blocking I/O
   - Event-driven using `poll()`
   - State machine for request processing

2. **Request Processing**
   - Binary protocol parsing
   - Command validation
   - Response generation

3. **Data Storage**
   - Custom hashmap implementation
   - Efficient key-value storage
   - Progressive resizing

### Client Architecture

1. **Command Processing**
   - Command-line argument parsing
   - Binary protocol formatting
   - Response handling

2. **Network Communication**
   - TCP socket handling
   - Request sending
   - Response receiving

## Performance Considerations

1. **Hashmap Design**
   - Progressive resizing for smooth performance
   - Efficient collision handling
   - Memory-efficient storage

2. **Network I/O**
   - Non-blocking operations
   - Efficient buffer management
   - Event-driven architecture

## Future Improvements

1. **Features**
   - Add more Redis-like commands
   - Implement data persistence
   - Add support for different data types
   - Add authentication

2. **Performance**
   - Implement connection pooling
   - Add request pipelining
   - Optimize memory management

3. **Reliability**
   - Add proper logging
   - Implement error recovery
   - Add health checks

## Contributing

Feel free to contribute to this project by:
1. Forking the repository
2. Creating a feature branch
3. Making your changes
4. Submitting a pull request

## License

This project is open source and available under the MIT License.

## Acknowledgments

- Inspired by Redis
- Built for educational purposes
- Demonstrates core system programming concepts 