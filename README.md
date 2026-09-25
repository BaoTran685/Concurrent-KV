# Concurrent Key-Value Store in C++

A multithreaded in-memory key-value store written in C++ to explore TCP networking, thread pools, synchronization, lock contention, concurrent data structure,
crash recovery, and write ahead logging (WAL).

## Features

- In-memory integer key-value store via `GET`, `SET`, and `DELETE` commands.
- TCP server built using POSIX sockets: `socket()`, `bind()`, `listen()`, `accept()`, `recv()`, and `send()`.
- Newline-delimited (`\n`) TCP command protocol for communicating between clients and the server.
- Fixed-size worker thread pool using the producer-consumer pattern to service multiple clients concurrently.
- Shared client queue coordinated using `std::mutex` and `std::conditional_variable`.
- Multiple synchronization strategies for the in-memory KV engine:
    - `std::mutex` *(global locking)*: global exclusive locking.
    - `std::shared_mutex` *(reader-writer locking)*: concurrent readers with exclusive writers.
    - Sharded `std::shared_mutex` *(sharded locking)*: parititions keys accross independently locked shards to reduce lock contention.
- Append-only Write-Ahead Log (WAL) for persistent storage of mutating operations (`SET` and `DELETE`).
- Crash recovery by replaying WAL records during server startup to reconstruct the in-memory state.
- Signal handling for graceful server shutdown.
- Multithreaded benchmark suite for comparison of global locking, reader-writer locking, and sharded locking.

---
## Architecture

The server is divided into four main components: the TCP networking layer, worker thread pool, concurrent in-memory key-value engine, and write-ahead logging layer.

### TCP Server
The main server thread listens for incoming TCP connections using POSIX sockets. When a client connects, its socket file descriptor (`client_fd`) is placed into a shared client queue.

### Worker Thread Pool
The server currently uses a fixed pool of three worker threads.

Workers wait on a `std::conditional_variable` while the client queue is empty. When a new connection is enqueued, `notify_one()` wakes one worker and the worker removes that `client_fd` from the queue and becomes responsible for servicing that client.

Each worker remains assigned to its client until the client disconnects. With three workers, the server can therefore service up to three client connections simultaneously. Additional connections remain in the queue until a worker becomes available.

### Concurrent In-Memory Engine
Commands received from clients are parsed and dispatched to the in-memory key-value store.

The project implements three sychronization strategies:
- Global Lock: one `std::mutex` protects the entire key-value map.
- Shared Lock: one `std::shared_mutex` allows multiple concurrent `GET` operations while `SET` and `DELETE` operations acquire exclusive access.
- Sharded Shared Lock: the key space is partitioned into `16` independently locked shards, allowing operations on different shards to run concurrently --> reduces lock contention.

These implementations are benchmarked against one another to study the performance trade-offs. Sharded shared lock is the best performer and is used as the default strategy for the engine.

### Write-Ahead Log (WAL)
Mutating operations (`SET` and `DELETE`) are persisted using an append-only Write-Ahead Log.

For a `SET` or `DELETE` operation, the server first serializes the mutation and appends it to the WAL file before applying the change to the in-memory key-value store.

We prioritize writing to WAL over making update to in-memory store because in case of a sudden shutdown, if the operation is written in WAL, we can recover it during the replay step.

`GET` operations do not modify database state and therefore do not need to be written to the WAL.

### Crash Recovery
The in-memory key-value store, as its name, only stays in RAM and when the process is terminated, all contents disappear. The WAL acts as a durable source of mutation history.

When the server starts, it reads the WAL sequentially and replays each valid `SET` and `DELETE` operation. Because mutations are replayed in the same order in which they were originally logged, the reconstructed in-memory database should reach the same state that existed before shutdown (they should have the same key-value pairs).

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

The current system supports persistent storage. Turning off the server will not lose all the in memory data but it is stored in file `kw.wal`. When turning on the server, the first thing the server does is to replay the operations from this file to populate its in-memory engine.

### Benchmark Program:

Yes you can run the benchmark yourself too! The below benchmark result is from my machine and your machine may produce different benchmark results.

```bash
cd src/benchmark
g++ -std=c++20 benchmark.cc ../sophistocated_program/kv_store.cc -I ../sophistocated_program/ -o prog
./prog
```

You can edit the global variables in the file `benchmark.cc` to edit the benchmark configurations. The default setting is:

- `KEY_RANGE = 1e5`: the range for the randomly generated keys
- `THREAD_COUNT = 4`: the number of threads running concurrently
- `OPERATIONS_PER_THREAD = 1e6`: the number of operations performed by each thread
- `TEST_ITERATIONS = 10`: the number of testing iterations


