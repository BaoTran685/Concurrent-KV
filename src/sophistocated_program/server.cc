#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <queue>

#include <thread>
#include <shared_mutex>
#include <csignal>

#include "persistent_kv_store.h"

// Number of workers supported.
// Rather than having one client -> one thread, which is bad because then 10K clients -> 10K threads,
//  resulting in using a lot of resources.
// We only use some threads to handle that big number of clients concurrently. For example, we can
//  have 3 workers working for client A, client B, client C requests. Then we may put some of these client
//  handlers to sleep to handle client D, client E, and so on requests.
// This is yet to be implemented. At the current code, we can only handle up to three clients at the same time.
// If one client leaves, then the next client in the queue can be processed.
constexpr int WORKERS = 3;

// For handling producer-consumer 
std::queue<int> client_queue;
std::mutex client_queue_mutex;
std::condition_variable client_queue_cv;
bool shutting_down = false;

// For handling server shutdown
// When we press CTRL + C in the server terminal, we wake up all workers to close them.
// Also, the client needs to exit as well for the server to shutdown.
volatile std::sig_atomic_t stop_requested = 0;
int server_fd = -1;
void handle_signal(int) {
    stop_requested = 1;
    if (server_fd != -1) {
        close(server_fd);
    }
}

// Data engine
std::unique_ptr<KVStore> engine = std::make_unique<ShardedSharedLock_KVStore>();
PersistentKVStore store {std::move(engine), "kv.wal"};

// process_command(line) takes in a line and determines if it is one of the three operations: GET, SET, or DELETE
//  and performs such operation. The function then returns the status of execution (SUCCESS or ERROR).
std::string process_command(const std::string& line) {
    std::istringstream iss(line);
    std::string command;
    iss >> command;

    if (command == "get") {
        int key;
        iss >> key;

        int result = store._get(key);
        if (result == INT_MAX) {
            return "ERROR: Key does not exist.";
        }
        return std::to_string(result);
    }
    else if (command == "set") {
        int key;
        int value;
        iss >> key >> value;

        store._set(key, value);
        return "SUCCESS: Operation set succeeded.";
    }
    else if (command == "delete") {
        int key;
        iss >> key;

        store._delete(key);
        return "SUCCESS: Operation delete succeeded.";
    }
    
    return "ERROR: Invalid command.";
}

// handle_client(client_fd) waits for the bytes coming from client_fd via recv function.
//  Then by the character '\n', we can separate the different commands.
void handle_client(int client_fd) {
    std::string pending_message;
    char buffer[1024];

    while (true) {
        // Wait for the bytes to come in, which is written to buffer.
        ssize_t bytes = recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes <= 0) {
            break;
        }

        // Append the bytes as characters to our pending_message, so we treat the previous bytes
        //  and current bytes as a whole.
        pending_message.append(buffer, bytes);

        // Process the pending_message: peeling each substrings separated by '\n' and process the substrings
        //  as one command.
        while (true) {
            size_t newline_position = pending_message.find('\n');
            if (newline_position == std::string::npos) {
                break;
            }

            std::string command = pending_message.substr(0, newline_position);
            pending_message.erase(0, newline_position + 1);

            std::string response = process_command(command);
            response += '\n';
            send(client_fd, response.data(), response.size(), 0);
        }
    }
    close(client_fd);
}

// worker(): creates a worker thread that is tied to one client.
// At first, when the code enters the {} part, it is blocked and put to wait until the condition variable notifies it.
//  Then, the code selects the first client and handles it. Since there are many workers running, how do we make sure that
//  only one worker takes the first client? This is when the lock comes in to play. It guarantees that this critical section
//  is run mutually exclusively.
// The reason why the code is in a while-loop is because when client A disconnects, we want to tie this worker to the next
//  client in the queue.
void worker() {
    while (true) {
        int client_fd;
        {
            std::unique_lock<std::mutex> lock(client_queue_mutex);

            // There is a predicate in the wait such that, we only want to continue from unblock when either
            //  the client queue is non-empty or when we are shutting down.
            client_queue_cv.wait(lock, [] {
                return !client_queue.empty() || shutting_down;
            });
            // In the case when the queue is empty and we are shutting down, we want to exit the loop and close the worker.
            if (client_queue.empty() && shutting_down) {
                break;
            }

            client_fd = client_queue.front();
            client_queue.pop();
        }
        handle_client(client_fd);
    }
}

int main() {
    // Handle interrupts
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    // TCP Server
    // 1. Create a socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Socket failed." << std::endl;
        return 1;
    }
    // 2. Enable the socket to be able to re-use previous port. Crucial for development stage
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        std::cerr << "Set Sock Opt failed." << std::endl;
        return 1;
    }
    // 3. Create an address and specify a port to bind to the socket
    sockaddr_in address {};
    address.sin_family = AF_INET;
    address.sin_port = htons(5555);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (sockaddr*) &address, sizeof(address)) == -1) {
        std::cerr << "Bind failed." << std::endl;
        return 1;
    }
    // 4. Make this socket the listening socket
    if (listen(server_fd, 10) == -1) {
        std::cerr << "Listen failed." << std::endl;
        return 1;
    }
    // TCP Server is now online
    std::cout << "Listening on port 5555..." << std::endl;
    
    // Create workers
    std::vector<std::thread> workers;
    for (int i = 0; i < WORKERS; ++i) {
        workers.push_back(std::thread(&worker));
    }

    // Accept clients. The code follows a producer-consumer pattern.
    //  Here, we are accepting clients from TCP, then putting it into a queue.
    //  Then, we notify one worker to handle this client and the worker is then responsible for
    //  that client.
    while (!stop_requested) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd == -1) {
            if (stop_requested) {
                break;
            }
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(client_queue_mutex);
            client_queue.push(client_fd);
        }

        client_queue_cv.notify_one();
    }

    // Shutdown
    {
        std::lock_guard<std::mutex> lock(client_queue_mutex);
        shutting_down = true;
    }
    client_queue_cv.notify_all();
    
    for (std::thread &worker : workers) {
        worker.join();
    }

    std::cout << "Shutting down..." << std::endl;
    return 0;
}



