#include <iostream>
#include <vector>
#include <unordered_map>
#include <queue>
#include <thread>
#include <future>

// Supported commands:
//  get <key> : Retrieve the value with the corresponding key.
//  set <key> <value> : If such key doesn't exist yet, create such key-value pair.
//      Otherwise, put the key-value pair to have such new value.
//  delete <key> : Delete such key-value pair if exists. Otherwise, do nothing.

// Note: <key> is of type int.
//       <value> is of type int.

// Concurrency: All GET tasks can be executed concurrently.
//  SET and DELETE tasks needed to be executed sequentially.


std::unordered_map<int, int> kv;
std::mutex kv_mutex;

int _get(int key) {
    auto it = kv.find(key);
    if (it == kv.end()) {
        // key doesn't exist
        return 0;
    }
    return it->second;
}

void _set(int key, int value) {
    std::lock_guard<std::mutex> lock(kv_mutex);
    kv[key] = value;
}

void _delete(int key) {
    std::lock_guard<std::mutex> lock(kv_mutex);
    kv.erase(key);
}

enum class Command {
    SET, GET, DELETE
};

struct Task {
    Command cmd;
    int key;
    int value;
};


int main() {
    std::queue<Task> task_queue;

    std::string command;
    while (std::cin >> command) {
        if (command == "get") {
            int key;
            std::cin >> key;
            task_queue.push(Task {Command::GET, key, 0});
        }
        else if (command == "set") {
            int key;
            int value;
            std::cin >> key >> value;
            task_queue.push(Task {Command::SET, key, value});
        }
        else if (command == "delete") {
            int key;
            std::cin >> key;
            task_queue.push(Task {Command::DELETE, key, 0});
        }
        else {
            std::cout << "Invalid command. Please try again..." << std::endl;
        }
    }


    while (!task_queue.empty()) {

        // Creating threads
        std::vector<std::thread> thread_vector;
        std::vector<std::future<int>> result_vector;
        if (task_queue.front().cmd == Command::GET) {
            while (!task_queue.empty() && task_queue.front().cmd == Command::GET) {

                std::packaged_task<int(int)> task(_get);
                result_vector.push_back(task.get_future());

                int key = task_queue.front().key;
                thread_vector.push_back(std::thread(std::move(task), key));

                task_queue.pop();
            }
        }
        else if (task_queue.front().cmd == Command::SET) {
            int key = task_queue.front().key;
            int value = task_queue.front().value;
            thread_vector.push_back(std::thread(&_set, key, value));
            task_queue.pop();
        }
        else if (task_queue.front().cmd == Command::DELETE) {
            int key = task_queue.front().key;
            thread_vector.push_back(std::thread(&_delete, key));
            task_queue.pop();
        }
        else {
            throw std::runtime_error("Invalid Command.");
        }

        // Joining threads
        for (std::thread &t : thread_vector) {
            t.join();
        }

        // Print results
        for (std::future<int> &r : result_vector) {
            std::cout << r.get() << std::endl;
        }

    }

    return 0;
}



