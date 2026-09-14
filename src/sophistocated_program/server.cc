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

#include "shared_lock.h"

// Number of workers supported.
// Rather than having one client -> one thread, which is bad because then 10K clients -> 10K threads,
//  resulting in using a lot of resources.
// We only use some threads to handle that big number of clients concurrently. For example, we can
//  have 3 workers working for client A, client B, client C requests. Then we may put some of these client
//  handlers to sleep to handle client D, client E, and so on requests.
constexpr int WORKERS = 3;

// For handling producer-consumer 
std::queue<int> client_queue;
std::mutex client_queue_mutex;
std::condition_variable client_queue_cv;

KVStore store;

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
void worker() {
    while (true) {
        int client_fd;
        {
            std::unique_lock<std::mutex> lock(client_queue_mutex);
            client_queue_cv.wait(lock);
            client_fd = client_queue.front();
            client_queue.pop();
        }
        handle_client(client_fd);
    }
}

int main() {
    
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Socket failed." << std::endl;
        return 1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        std::cerr << "Set Sock Opt failed." << std::endl;
        return 1;
    }

    sockaddr_in address {};
    address.sin_family = AF_INET;
    address.sin_port = htons(5555);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (sockaddr*) &address, sizeof(address)) == -1) {
        std::cerr << "Bind failed." << std::endl;
        return 1;
    }

    if (listen(server_fd, 10) == -1) {
        std::cerr << "Listen failed." << std::endl;
        return 1;
    }

    std::cout << "Listening on port 5555..." << std::endl;
    
    std::vector<std::thread> workers;
    for (int i = 0; i < WORKERS; ++i) {
        workers.push_back(std::thread(&worker));
    }

    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd == -1) {
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(client_queue_mutex);
            client_queue.push(client_fd);
        }

        client_queue_cv.notify_one();
    }

    close(server_fd);

    return 0;
}



