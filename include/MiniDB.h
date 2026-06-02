#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <unordered_map>
#include <fstream>
#include <cstdint>
#include <vector>
#include <shared_mutex>
#include <mutex>

namespace minidb {

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
     * @param sync If true, forces OS flush to disk.
     * @return True if successful, false otherwise.
     */
    bool Put(std::string_view key, std::string_view value, bool sync = false);

    /**
     * @brief Retrieves the value associated with a key.
     * @param key The key to look up.
     * @return Optional containing the value if found, empty if not found or deleted.
     */
    std::optional<std::string> Get(std::string_view key);

    /**
     * @brief Deletes a key from the database.
     * @param key The key to delete.
     * @param sync If true, forces OS flush to disk.
     * @return True if successful, false otherwise.
     */
    bool Delete(std::string_view key, bool sync = false);

    /**
     * @brief Compacts the active database log to reclaim space from updated/deleted records.
     * @return True if successful, false otherwise.
     */
    bool Compact();

    /**
     * @brief Manually synchronizes the underlying file stream buffers to OS/disk.
     * @return True if successful.
     */
    bool Sync();

private:
    /**
     * @brief Binary protocol layout for the log file
     */
    #pragma pack(push, 1) // Ensure no padding
    struct RecordHeader {
        uint32_t magic;      // Magic bytes "MDB2"
        uint32_t header_crc; // CRC32 of timestamp, tombstone, key_len, val_len, and the KEY string
        uint32_t value_crc;  // CRC32 of the VALUE string
        uint64_t timestamp;  // Epoch time in ms
        uint8_t tombstone;   // 0x00 = Active, 0x01 = Deleted
        uint32_t key_len;    // Length of the key part
        uint32_t val_len;    // Length of the value part
    };
    #pragma pack(pop)

    static constexpr uint32_t MAGIC_BYTES = 0x3242444D; // "MDB2" in little-endian roughly

    std::string db_path_;
    std::fstream data_file_; // Used for appends
    
    // Hash Index mapping Key to its latest file offset
    std::unordered_map<std::string, std::streampos> key_dir_; 

    // Readers-Writer lock for concurrent reads (Get) and exclusive writes (Put/Delete/Compact)
    mutable std::shared_mutex db_mutex_;

    // Reusable write buffer to avoid heap allocations on every AppendRecord
    std::vector<char> write_buffer_;

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
    bool AppendRecord(std::string_view key, std::string_view value, bool is_tombstone, bool sync, std::streampos& offset);
};

} // namespace minidb
