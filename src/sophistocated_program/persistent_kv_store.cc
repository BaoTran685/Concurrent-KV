#include "persistent_kv_store.h"


// CTOR of PersistentKVStore: this is a wrapper of both the WAL and the in-memory concurrent kv store.
// When we first create this object, we want to replay the operations from the WAL file to populate our 
//  in memory kv store.
// When an operation is called, we always want to write that operation to the log before executing that operation.
//  That's why the name is Write Ahead Log.
PersistentKVStore::PersistentKVStore(std::unique_ptr<KVStore> engine, const std::string& wal_path)
    : engine{std::move(engine)}, wal{wal_path} {

    wal.replay([this](const WalRecord& record) {
        if (record.operation == WalOperation::SET) {
            this->engine->_set(record.key, record.value);
        }
        else if (record.operation == WalOperation::DELETE) {
            this->engine->_delete(record.key);
        }
    });
}

int PersistentKVStore::_get(int key) {
    return engine->_get(key);
}

void PersistentKVStore::_set(int key, int value) {
    wal.appendSet(key, value);
    engine->_set(key, value);
}

void PersistentKVStore::_delete(int key) {
    wal.appendDelete(key);
    engine->_delete(key);
}
