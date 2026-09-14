#include <array>
#include <unordered_map>
#include <shared_mutex>

class KVStore {
private:
    static constexpr int SHARDS = 16;

    struct Shard {
        std::unordered_map<int, int> data;
        std::shared_mutex mutex;
    };

    std::array<Shard, SHARDS> shards;

    size_t get_shard_index(int key) const;
public:
    int _get(int key); // mutually exclusive
    void _set(int key, int value); // mutually exclusive
    void _delete(int key); // mutually exclusive
};
