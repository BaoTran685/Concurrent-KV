#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>


std::unordered_map<int, int> kv;

std::string process_command(const std::string& line) {
    std::istringstream iss(line);
    std::string command;
    iss >> command;

    if (command == "get") {
        int key;
        iss >> key;

        auto it = kv.find(key);
        if (it == kv.end()) {
            return "ERROR: Key not found.";
        }
        return std::to_string(it->second);
    }
    else if (command == "set") {
        int key;
        int value;
        iss >> key >> value;

        kv[key] = value;
        return "SUCCESS: Operation set succeeded.";
    }
    else if (command == "delete") {
        int key;
        iss >> key;

        kv.erase(key);
        return "SUCCESS: Operation delete succeeded.";
    }
    
    return "ERROR: Invalid command.";
}

void handle_receive(int client_fd) {
    std::string pending_message;
    char buffer[1024];

    while (true) {
        ssize_t bytes = recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes <= 0) {
            break;
        }

        pending_message.append(buffer, bytes);

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
    
    int client_fd = accept(server_fd, nullptr, nullptr);
    if (client_fd == -1) {
        std::cerr << "Accept failed." << std::endl;
        return 1;
    }

    handle_receive(client_fd);

    close(client_fd);
    close(server_fd);

    return 0;
}



