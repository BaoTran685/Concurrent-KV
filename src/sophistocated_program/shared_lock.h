#include <unordered_map>
#include <thread>
#include <shared_mutex>

class KVStore {
private:
    std::unordered_map<int, int> data;
    std::shared_mutex mutex;
public:
    int _get(int key); // shared mutex lock
    void _set(int key, int value); // unique mutex lock
    void _delete(int key); // unique mutex lock
};
