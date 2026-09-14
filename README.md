# Concurrent Key-Value Store in C++

A multithreaded in-memory key-value store written in C++ to explore **TCP networking, thread pools, synchronization, lock contention, and concurrent data structure**.

## Features

- In-memory integer key-value store via `GET`, `SET`, and `DELETE` commands
- TCP server using POSIX sockets: `socket()`, `bind()`, `listen()`, `accept()`, `recv()`
- Newline-delimited (`\n`) TCP command protocol
- Fixed 3-worker pool using the producer-consumer pattern to service clients
- Thread synchronization using:
    - `std::mutex` *(global locking)*
    - `std::shared_mutex` *(reader-writer locking)*
    - sharded `std::shared_mutex` *(lock sharding)*
- Signal handling for graceful server shutdown
- Multithreaded benchmark suite for comparison of global locking, reader-writer locking, and lock sharding

---
## Architecture

The current server uses a fixed pool of three worker threads.

The main server thread accepts incoming TCP connections and place their `client_fd` into a shared queue.

Worker threads wait on a `std::condition_variable`. When a client becomes available, `notify_one()` is invoked and so one worker removes
the client from the queue and becomes responsible for that connection.

Each worker remains assigned to its client until that client disconnects.

Therefore, the current implementation supports up to **three persistent client connections simultanously**. Aditional clients remain in the
queue until a worker becomes available.

---