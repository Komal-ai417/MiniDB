#pragma once

#include <string>
#include <unordered_map>
#include <fstream>
#include <cstdint>
#include <vector>
#include <atomic>

namespace minidb {

class SpinLock {
    std::atomic_flag locked = ATOMIC_FLAG_INIT;
public:
    void lock() {
        while (locked.test_and_set(std::memory_order_acquire)) { ; }
    }
    void unlock() {
        locked.clear(std::memory_order_release);
    }
};

template <typename T>
class LockGuard {
    T& mutex_;
public:
    explicit LockGuard(T& m) : mutex_(m) { mutex_.lock(); }
    ~LockGuard() { mutex_.unlock(); }
    LockGuard(const LockGuard&) = delete;
    LockGuard& operator=(const LockGuard&) = delete;
};

class MiniDB {
public:
    /**
     * @brief Opens or creates a MiniDB database at the specified path.
     * @param db_path The file path for the database log file.
     */
    explicit MiniDB(const std::string& db_path);
    ~MiniDB();

    // Prevent copying
    MiniDB(const MiniDB&) = delete;
    MiniDB& operator=(const MiniDB&) = delete;

    /**
     * @brief Inserts or updates a key-value pair in the database.
     * @param key The key to insert.
     * @param value The value associated with the key.
     * @return True if successful, false otherwise.
     */
    bool Put(const std::string& key, const std::string& value);

    /**
     * @brief Retrieves the value associated with a key.
     * @param key The key to look up.
     * @param value Output parameter for the retrieved value.
     * @return True if found, false if not found or deleted.
     */
    bool Get(const std::string& key, std::string& value);

    /**
     * @brief Deletes a key from the database.
     * @param key The key to delete.
     * @return True if successful, false otherwise.
     */
    bool Delete(const std::string& key);

    /**
     * @brief Compacts the active database log to reclaim space from updated/deleted records.
     * @return True if successful, false otherwise.
     */
    bool Compact();

private:
    /**
     * @brief Binary protocol layout for the log file
     */
    #pragma pack(push, 1) // Ensure no padding
    struct RecordHeader {
        uint32_t magic;      // Magic bytes "MDB1"
        uint32_t checksum;   // CRC32 of the REST of the record (from timestamp onwards)
        uint64_t timestamp;  // Epoch time in ms
        uint8_t tombstone;   // 0x00 = Active, 0x01 = Deleted
        uint32_t key_len;    // Length of the key part
        uint32_t val_len;    // Length of the value part
    };
    #pragma pack(pop)

    static constexpr uint32_t MAGIC_BYTES = 0x3142444D; // "MDB1" en little-endian roughly

    std::string db_path_;
    std::fstream data_file_;
    
    // Hash Index mapping Key to its latest file offset
    std::unordered_map<std::string, std::streampos> key_dir_; 

    // Custom lock implementing extreme zero-dependency thread-safety
    SpinLock db_mutex_;

    /**
     * @brief Recovers the hash index by reading the file sequentially on startup.
     * @return True if recovery succeeds (even partially over corrupt data), false on fatal IO error.
     */
    bool Recover();

    /**
     * @brief Calculates a standard CRC32 checksum via lookup table.
     * @param data Pointer to data buffer.
     * @param length Length of data.
     * @param previous_crc Previous CRC for chaining.
     * @return The 32-bit CRC checksum.
     */
    static uint32_t CalculateCRC32(const uint8_t* data, size_t length, uint32_t previous_crc = 0);
    
    /**
     * @brief Internal helper to append a record to the file.
     * @param key Key string.
     * @param value Value string.
     * @param is_tombstone True if this is a delete marker.
     * @param offset Output parameter to receive the written file offset.
     * @return True if successfully written.
     */
    bool AppendRecord(const std::string& key, const std::string& value, bool is_tombstone, std::streampos& offset);
};

} // namespace minidb
