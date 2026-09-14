#include <unordered_map>
#include <thread>

class KVStore {
private:
    std::unordered_map<int, int> data;
    std::mutex mutex;
public:
    int _get(int key); // mutually exclusive
    void _set(int key, int value); // mutually exclusive
    void _delete(int key); // mutually exclusive
};
