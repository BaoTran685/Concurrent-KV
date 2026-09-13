#include <iostream>
#include <vector>
#include <unordered_map>
#include <thread>
#include <functional>

// Supported commands:
//  get <key> : Retrieve the value with the corresponding key.
//  set <key> <value> : If such key doesn't exist yet, create such key-value pair.
//      Otherwise, put the key-value pair to have such new value.
//  delete <key> : Delete such key-value pair if exists. Otherwise, do nothing.

// Note: <key> is of type int.
//       <value> is of type int.


int _get(std::unordered_map<int, int> &kv, int key) {
    return kv[key];
}

void _set(std::unordered_map<int, int> &kv, int key, int value) {
    kv[key] = value;
}

void _delete(std::unordered_map<int, int> &kv, int key) {
    kv.erase(key);
}

enum class Command {
    SET, GET, DELETE
};

struct Operation {
    Command cmd;
    int key;
    int value;
};


int main() {
    std::unordered_map<int, int> kv;
    std::vector<Operation> operations;

    std::string command;
    while (std::cin >> command) {
        if (command == "get") {
            int key;
            std::cin >> key;
            operations.push_back(Operation {Command::GET, key, 0});
        }
        else if (command == "set") {
            int key;
            int value;
            std::cin >> key >> value;
            operations.push_back(Operation {Command::SET, key, value});
        }
        else if (command == "delete") {
            int key;
            std::cin >> key;
            operations.push_back(Operation {Command::DELETE, key, 0});
        }
        else {
            std::cout << "Invalid command. Please try again..." << std::endl;
        }
    }

    std::vector<std::thread> threads;
    for (Operation &op : operations) {
        if (op.cmd == Command::GET) {
            threads.push_back(std::thread (&_get, std::ref(kv), op.key));
        }
        else if (op.cmd == Command::SET) {
            threads.push_back(std::thread (&_set, std::ref(kv), op.key, op.value));
        }
        else if (op.cmd == Command::DELETE) {
            threads.push_back(std::thread (&_delete, std::ref(kv), op.key));
        }
    }

    for (std::thread &t : threads) {
        t.join();
    }

    return 0;
}



