#include "kv_store.h"

#include <mutex>
#include <shared_mutex>

#include <unordered_map>
#include <array>

// GlobalLock_KVStore: This implementation uses one mutex for all GET, SET, and DELETE commands.
// Therefore, all operations run mutually exclusively. -> Not quite efficient.
int GlobalLock_KVStore::_get(int key) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = data.find(key);
    if (it == data.end()) {
        return INT_MAX;
    }
    return it->second;
}

void GlobalLock_KVStore::_set(int key, int value) {
    std::lock_guard<std::mutex> lock(mutex);
    data[key] = value;
}

void GlobalLock_KVStore::_delete(int key) {
    std::lock_guard<std::mutex> lock(mutex);
    data.erase(key);
}

// SharedLock_KVStore: This implementatation introduces shared mutex where there are two types of lock:
//  a shared lock and a unique lock. By doing so, operations using a shared lock can run concurrently
//  whereas operations using unique lock run mutually exclusively.
// We use this approach to enable concurrent GET commands. -> Quite efficient.
int SharedLock_KVStore::_get(int key) {
    std::shared_lock<std::shared_mutex> lock(mutex);
    auto it = data.find(key);
    if (it == data.end()) {
        return INT_MAX;
    }
    return it->second;
}

void SharedLock_KVStore::_set(int key, int value) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    data[key] = value;
}

void SharedLock_KVStore::_delete(int key) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    data.erase(key);
}

// ShardedSharedLock_KVStore: This implementation splits our data (key-value pairs) into many parts/shards
//  so that operatings accessing different shards can run concurrently. -> More efficient.
size_t ShardedSharedLock_KVStore::get_shard_index(int key) const {
    return std::hash<int>{}(key) % SHARDS;
}

int ShardedSharedLock_KVStore::_get(int key) {
    Shard &shard = shards.at(get_shard_index(key));

    std::shared_lock<std::shared_mutex> lock(shard.mutex);
    auto it = shard.data.find(key);
    if (it == shard.data.end()) {
        return INT_MAX;
    }
    return it->second;
}

void ShardedSharedLock_KVStore::_set(int key, int value) {
    Shard &shard = shards.at(get_shard_index(key));

    std::unique_lock<std::shared_mutex> lock(shard.mutex);
    shard.data[key] = value;
}

void ShardedSharedLock_KVStore::_delete(int key) {
    Shard &shard = shards.at(get_shard_index(key));

    std::unique_lock<std::shared_mutex> lock(shard.mutex);
    shard.data.erase(key);
}
