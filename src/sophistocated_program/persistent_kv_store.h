
#include "kv_store.h"
#include "wal.h"

#include <memory>
#include <mutex>
#include <string>

class PersistentKVStore {
public:
    PersistentKVStore(std::unique_ptr<KVStore> engine, const std::string& wal_path);

    int _get(int key);
    void _set(int key, int value);
    void _delete(int key);
private:
    std::unique_ptr<KVStore> engine;
    WriteAheadLog wal;
};
