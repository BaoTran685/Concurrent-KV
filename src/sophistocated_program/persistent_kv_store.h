
#include "kv_store.h"
#include "wal.h"

#include <memory>
#include <mutex>
#include <string>

class PersistentKVSotre : public KVStore {
public:
    PersistentKVSotre(std::unique_ptr<KVStore> engine, const std::string& wal_path);

    int _get(int key) override;
    void _set(int key, int value) override;
    void _delete(int key) override;
private:
    std::unique_ptr<KVStore> engine;
    WriteAheadLog wal;
};
