# Concurrent Key-Value Store in C++

A multithreaded in-memory key-value store written in C++ to explore **TCP networking, thread pools, synchronization, lock contention, and concurrent data structure**.

## Features

- In-memory integer key-value store via `GET`, `SET`, and `DELETE` commands.
- TCP server using POSIX sockets: `socket()`, `bind()`, `listen()`, `accept()`, `recv()`.
- Newline-delimited (`\n`) TCP command protocol.
- Fixed 3-worker pool using the producer-consumer pattern to service clients. Can configure to have more workers.
- Thread synchronization using one of:
    - `std::mutex` *(global locking)*
    - `std::shared_mutex` *(reader-writer locking)*
    - sharded `std::shared_mutex` *(sharded locking)*.
- Signal handling for graceful server shutdown.
- Multithreaded benchmark suite for comparison of global locking, reader-writer locking, and sharded locking.

---
## Architecture

The current server uses a fixed pool of three worker threads.

The main server thread accepts incoming TCP connections and place their `client_fd` into a shared queue.

Worker threads wait on a `std::condition_variable`. When a client becomes available, `notify_one()` is invoked and so one worker removes the client from the queue and becomes responsible for that connection.

Each worker remains assigned to its client until that client disconnects.

Therefore, the current implementation supports up to **three persistent client connections simultanously**. Aditional clients remain in the queue until a worker becomes available.

---
## Benchmark




---
## How to run?

### Naive Program:

To compile and run program:
```bash
cd src/naive_program
g++ -std=c++20 naive_program.cc -o prog
./prog
```

Supported commands:
```text
get <key> : Return the value associated with such key.
set <key> <value> : Set the key-value pair into our store.
delete <key> : Delete the key-value pair having such key.
```

### TCP Server Program:

To complile and run program:
```bash
cd src/sophistocated_program
g++ -std=c++20 server.cc kv_store.cc -o prog
./prog
```

Then to create a client connection, open a new terminal and prompt:
```bash
nc localhost 5555
```

Then type in the commands like the ones supported in the above naive program `get`, `set`, and `delete` to send request to server.

The current implementaion supports up to three simultaneous clients.

### Benchmark Program:

Yes you can run the benchmark yourself too! The above benchmark result is run on my machine and it can produce different results compared to your machine.