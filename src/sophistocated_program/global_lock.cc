// Version 1 of the Data Engine.
// This version uses a global mutex for all GET, SET, and DELETE commands.
// As result, only one thread/worker is allowed to do a operation at a time.

#include "global_lock.h"
#include <thread>

// KVStore::_get(key) retrieves the value tied to such key if exists. Otherwise return INT_MAX.
// This operation is mutually exclusive.
int KVStore::_get(int key) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = data.find(key);
    if (it == data.end()) {
        return INT_MAX;
    }
    return it->second;
}

// KVStore::_set(key, value) puts the key-value pair into our data structure.
//  This operation is mutually exclusive.
void KVStore::_set(int key, int value) {
    std::lock_guard<std::mutex> lock(mutex);
    data[key] = value;
}

// KVStore::_delete(key) deletes such key-value pair.
//  This operation is mutually exclusive.
void KVStore::_delete(int key) {
    std::lock_guard<std::mutex> lock(mutex);
    data.erase(key);
}
