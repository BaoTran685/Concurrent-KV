#include <functional>
#include <mutex>
#include <string>

enum class WalOperation {
    SET, DELETE
};

struct WalRecord {
    WalOperation operation;
    int key;
    int value;
};

class WriteAheadLog {
public:
    explicit WriteAheadLog(const std::string& path);
    ~WriteAheadLog();

    WriteAheadLog(const WriteAheadLog&) = delete; // disable copy ctor
    WriteAheadLog& operator=(const WriteAheadLog&) = delete; // disable copy operator

    void appendSet(int key, int value);
    void appendDelete(int key);

    void replay(const std::function<void(const WalRecord&)>& apply);

private:
    void append(const std::string& record);
    void writeAll(const char* data, size_t size);

    std::string path;
    int file_fd;
    std::mutex wal_mutex;
};



