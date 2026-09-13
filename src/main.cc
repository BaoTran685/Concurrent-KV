#include <iostream>
#include <unordered_map>

// Supported commands:
//  get <key> : Retrieve the value with the corresponding key.
//  set <key> <value> : If such key doesn't exist yet, create such key-value pair.
//      Otherwise, put the key-value pair to have such new value.
//  delete <key> : Delete such key-value pair if exists. Otherwise, do nothing.

// Note: <key> is of type 32-bit unsigned int.
//       <value> is of type 32-bit int.


class Concurrent_KV {
    std::unordered_map<int, int> m;

  public:
    int _get(int key);
    void _set(int key, int value);
    void _delete(int key);
};

int Concurrent_KV::_get(int key) {
    return m[key];
}

void Concurrent_KV::_set(int key, int value) {
    m[key] = value;
}

void Concurrent_KV::_delete(int key) {
    m.erase(key);
}

int main() {
    Concurrent_KV kv;

    std::string command;
    while (std::cin >> command) {
        if (command == "get") {
            int key;
            std::cin >> key;

            int value =kv._get(key);
            std::cout << value << std::endl;
        }
        else if (command == "set") {
            int key;
            int value;
            std::cin >> key >> value;

            kv._set(key, value);
        }
        else if (command == "delete") {
            int key;
            std::cin >> key;

            kv._delete(key);
        }
        else {
            std::cout << "Invalid command. Please try again..." << std::endl;
        }
    }


    return 0;
}



