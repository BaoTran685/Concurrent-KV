#ifndef KV_STORE_H
#define KV_STORE_H

#include <unordered_map>
#include <array>
#include <mutex>
#include <shared_mutex>


class KVStore {
public:
    virtual ~KVStore() = default;
    virtual int _get(int key) = 0;
    virtual void _set(int key, int value) = 0;
    virtual void _delete(int key) = 0;
};


class GlobalLock_KVStore : public KVStore {
    std::unordered_map<int, int> data;
    std::mutex mutex;
public:
    int _get(int key) override;
    void _set(int key, int value) override;
    void _delete(int key) override;
};


class SharedLock_KVStore : public KVStore {
    std::unordered_map<int, int> data;
    std::shared_mutex mutex;
public:
    int _get(int key) override;
    void _set(int key, int value) override;
    void _delete(int key) override;
};


class ShardedSharedLock_KVStore : public KVStore {
private:
    static constexpr int SHARDS = 16;

    struct Shard {
        std::unordered_map<int, int> data;
        std::shared_mutex mutex;
    };

    std::array<Shard, SHARDS> shards;

    size_t get_shard_index(int key) const;
public:
    int _get(int key) override;
    void _set(int key, int value) override;
    void _delete(int key) override;
};

#endif
