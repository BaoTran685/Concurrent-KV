// Version 2 of Data Engine.
// This version allows concurrent reads to the data as we use a shared mutex.
// However, SET and DELETE operations are still mutually exclusive.

#include "shared_lock.h"
#include <thread>
#include <shared_mutex>

// KVStore::_get(key) retrieves the value tied to such key if exists. Otherwise return INT_MAX.
// This operation is concurrent for all GETs.
int KVStore::_get(int key) {
    std::shared_lock<std::shared_mutex> lock(mutex);
    auto it = data.find(key);
    if (it == data.end()) {
        return INT_MAX;
    }
    return it->second;
}

// KVStore::_set(key, value) puts the key-value pair into our data structure.
//  This operation is mutually exclusive.
void KVStore::_set(int key, int value) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    data[key] = value;
}

// KVStore::_delete(key) deletes such key-value pair.
//  This operation is mutually exclusive.
void KVStore::_delete(int key) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    data.erase(key);
}
