#include "wal.h"

#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <fstream>
#include <sstream>


// CTOR of WriteAheadLog: we use this class to handle write ahead logging. For each operations called
//  that can mutate the in-memory store, we first append those commands into a file before updating the store.
// And when we first run the server, the class will read in such WAL file and execute those commands to
//  populate the in-memory store.
// We try to make the 
WriteAheadLog::WriteAheadLog(const std::string& path) : path{path} {
    file_fd = ::open(path.c_str(), O_CREAT | O_RDWR | O_APPEND, 0644); // 0644 -> rw-r--r--

    if (file_fd == -1) {
        throw std::system_error(errno, std::generic_category(), "Failed to open WAL file.");
    }
}

// DTOR of WriteAheadLog: we have to close the file descriptor to release the resource back to OS.
WriteAheadLog::~WriteAheadLog() {
    if (file_fd != -1) {
        ::close(file_fd);
    }
}

void WriteAheadLog::appendSet(int key, int value) {
    std::string record = "S " + std::to_string(key) + " " + std::to_string(value) + "\n";
    append(record);
}

void WriteAheadLog::appendDelete(int key) {
    std::string record = "D " + std::to_string(key) + "\n";
    append(record);
}

// append(record) creates a lock so that only one thread can write to our WAL file at a time.
// We want to call fsync so that the file contents are written directly to storage, and don't just
//  stay in OS page cache. This is because if the file contents just stay in cache and something
//  bad hapens and the system shuts down, we would lose the content.
// However, this approach can be quite slow. Later on we will immplement group fsync.
void WriteAheadLog::append(const std::string& record) {
    std::lock_guard<std::mutex> lock(wal_mutex);

    writeAll(record.data(), record.size());

    if (::fsync(file_fd) == -1) {
        throw std::system_error(errno, std::generic_category(), "Failed to fsync WAL file.");
    }
}

// writeAll(data, size) will ensure that we write all contents of data into the file.
// POSIX write() is allowed to write fewer bytes than we requested so we have to put this in a 
//  while loop that exits after all bytes have been written.
void WriteAheadLog::writeAll(const char* data, size_t size) {
    size_t total_written = 0;

    while (total_written < size) {
        ssize_t bytes_written = ::write(file_fd, data + total_written, size - total_written);

        if (bytes_written == -1) {
            if (errno == EINTR) {
                // interrupted by a signal before copmleting -> try again
                continue;
            }
            throw std::system_error(errno, std::generic_category(), "Failed to write WAL record.");
        }

        total_written += static_cast<size_t>(bytes_written);
    }
}

// replay(apply) opens the file using ifstream. Note that when we perform write ahead logging, we want to use
//  POSIX APIs (open,write,fsync,close) to perform because we want persistency.
// For replaying, we only need to read in the file lines and for each line we determine what kind of command it is,
//  invoke apply() to apply it to the in-memory store.
// An interesting feature implemented here is that we can handle crash recovery. Let say for example in the previous
//  run we are unable to write the full command "SET 10 100" and only able to write "SET 1". Then the system tracks
//  it as an imcomplete operation and removes it.
void WriteAheadLog::replay(const std::function<void(const WalRecord&)>& apply) {
    std::ifstream input (path, std::ios::binary);

    if (!input.is_open()) {
        throw std::runtime_error("Failed to open WAL file for replay.");
    }

    off_t valid_bytes = 0; // for truncating an imcomplete final line

    std::string line;
    while (std::getline(input, line)) {
        if (input.eof()) {
            break;
        }

        std::streampos current_position = input.tellg();
        if (current_position == std::streampos(-1)) {
            throw std::runtime_error("Could not determine WAL position.");
        }
        valid_bytes = static_cast<off_t>(current_position);

        std::istringstream parser (line);

        char operation;
        parser >> operation;

        if (operation == 'S') {
            int key;
            int value;

            if (!(parser >> key >> value)) {
                throw std::runtime_error("Corrupted SET WAL record: " + line + ".");
            }

            parser >> std::ws;

            if (!parser.eof()) {
                throw std::runtime_error("Unexpected data in SET WAL record: " + line + ".");
            }

            apply(WalRecord {WalOperation::SET, key, value});
        }
        else if (operation == 'D') {
            int key;

            if (!(parser >> key)) {
                throw std::runtime_error("Corrupted DELETE WAL recrod: " + line + ".");
            }

            parser >> std::ws;

            if (!parser.eof()) {
                throw std::runtime_error("Unexpected data in DELETE WAL record: " + line + ".");
            }

            apply(WalRecord {WalOperation::DELETE, key, 0});
        }
        else {
            throw std::runtime_error("Unknown WAL operation: " + line + ".");
        }
    }

    if (::ftruncate(file_fd, valid_bytes) == -1) {
        throw std::system_error(errno, std::generic_category(), "Failed to truncate imcomplete WAL record.");
    }

    if (::fsync(file_fd) == -1) {
        throw std::system_error(errno, std::generic_category(), "Failed to sync WAL after recovery.");
    }
}
