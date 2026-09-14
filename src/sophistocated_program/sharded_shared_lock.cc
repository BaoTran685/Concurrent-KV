// Version 3 of the Data Engine.
// This versions is an improvement of the "shared_lock.cc/.h" version where
//  the GET commands can run concurrently, and SET and DELETE commands are still mutually exclusive.
// The only difference is that, we now split the data into n parts and use the hash value of the key
//  for the indexing. By doing so, we can enable concurrent SET and DELETE if they happen to access
//  different parts of our data.

#include "sharded_shared_lock.h"
#include <shared_mutex>

// get_shard_index(key) returns the index of the shard/part of the data such that the key-value
//  pair should go in.
size_t KVStore::get_shard_index(int key) const {
    return std::hash<int>{}(key) % SHARDS;
}

// KVStore::_get(key) retrieves the value tied to such key if exists. Otherwise return INT_MAX.
// This operation is concurrent for all GETs.
int KVStore::_get(int key) {
    Shard &shard = shards.at(get_shard_index(key));

    std::shared_lock<std::shared_mutex> lock(shard.mutex);
    auto it = shard.data.find(key);
    if (it == shard.data.end()) {
        return INT_MAX;
    }
    return it->second;
}

// KVStore::_set(key, value) puts the key-value pair into our data structure.
//  This operation is mutually exclusive. However, for operations accessing different
//  shards, they are concurrent.
void KVStore::_set(int key, int value) {
    Shard &shard = shards.at(get_shard_index(key));

    std::unique_lock<std::shared_mutex> lock(shard.mutex);
    shard.data[key] = value;
}

// KVStore::_delete(key) deletes such key-value pair.
//  This operation is mutually exclusive. However, for oprerations accessing different
//  shards, they are concurrent.
void KVStore::_delete(int key) {
    Shard &shard = shards.at(get_shard_index(key));

    std::unique_lock<std::shared_mutex> lock(shard.mutex);
    shard.data.erase(key);
}