// Version 1: Naive Program
// This is a pretty naive program where the only concurrency part is when we
//  have multiple GET commands consecutively. Only then, we would try to run all
//  the GET tasks concurrently. SET and DELETE tasks are run sequentially.

// For example, let say we have the following tasks: GET GET SET GET DELETE.
//  Then, {GET, GET} would be run concurrently using two threads, then after
//  those two threads finish, we create a new thread for the SET task, and then
//  a new thread for the GET task, and finally a new thread for the DELETE task.

#include <iostream>
#include <vector>
#include <unordered_map>
#include <queue>
#include <thread>
#include <future>

// Supported commands:
//  get <key> : Retrieve the value with the corresponding key.
//  set <key> <value> : Set the key-value pair with such key to have such value.
//  delete <key> : Delete such key-value pair.

// Note: <key> is of type int.
//       <value> is of type int.

// Concurrency: All GET tasks can be executed concurrently.
//  SET and DELETE tasks needed to be executed sequentially.

std::unordered_map<int, int> kv;
std::mutex kv_mutex;

// Supported commands: GET, SET, DELETE.
enum class Command {
    GET, SET, DELETE
};

// Task is comprises of a command (either GET, SET, or DELETE), a key and a value.
struct Task {
    Command cmd;
    int key;
    int value;
};

// _get(key) returns the value of the key-value pair with such key.
int _get(int key) {
    auto it = kv.find(key);
    if (it == kv.end()) {
        return 0;
    }
    return it->second;
}

// _set(key, value) modifies kv where it sets the key-value with such key to have such value.
void _set(int key, int value) {
    std::lock_guard<std::mutex> lock(kv_mutex);
    kv[key] = value;
}

// _delete(key) modifies kv where it erases the key-value pair with such key.
void _delete(int key) {
    std::lock_guard<std::mutex> lock(kv_mutex);
    kv.erase(key);
}


int main() {
    std::queue<Task> task_queue;

    // Retrieve input from std::cin, convert them to tasks, and then put them into a queue.
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

    // Run each task in the queue.
    while (!task_queue.empty()) {

        // Create threads.
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

        // Join threads.
        for (std::thread &t : thread_vector) {
            t.join();
        }

        // Print results. Note that this is only applicable for GET commands.
        for (std::future<int> &r : result_vector) {
            std::cout << r.get() << std::endl;
        }

    }

    return 0;
}


