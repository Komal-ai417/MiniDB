#include "MiniDB.h"

#include <chrono>
#include <cstring>
#include <iostream>
#include <vector>

namespace minidb {

MiniDB::MiniDB(const std::string& db_path) : db_path_(db_path) {
    // Ensure the file exists, create empty if not
    std::ifstream check_file(db_path_.c_str());
    if (!check_file.good()) {
        std::ofstream create_file(db_path_.c_str(), std::ios::out | std::ios::binary);
        create_file.close();
    }
    check_file.close();

    // Open file in read/write binary mode
    data_file_.open(db_path_, std::ios::in | std::ios::out | std::ios::binary | std::ios::app);
    if (!data_file_.is_open()) {
        throw std::runtime_error("Failed to open DB file: " + db_path_);
    }

    // Recover index from file
    if (!Recover()) {
        std::cerr << "Warning: Recovery encountered an error or a corrupt record in " << db_path_ << "\n";
    }
}

MiniDB::~MiniDB() {
    LockGuard<SpinLock> lock(db_mutex_);
    if (data_file_.is_open()) {
        data_file_.flush();
        data_file_.close();
    }
}

static const uint32_t* GetCRC32Table() {
    static uint32_t table[256];
    static bool initialized = false;
    if (!initialized) {
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t crc = i;
            for (uint32_t j = 0; j < 8; j++) {
                crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320 : crc >> 1;
            }
            table[i] = crc;
        }
        initialized = true;
    }
    return table;
}

uint32_t MiniDB::CalculateCRC32(const uint8_t* data, size_t length, uint32_t previous_crc) {
    uint32_t crc = ~previous_crc;
    const uint32_t* table = GetCRC32Table();
    for (size_t i = 0; i < length; ++i) {
        crc = (crc >> 8) ^ table[(crc ^ data[i]) & 0xFF];
    }
    return ~crc;
}

bool MiniDB::AppendRecord(const std::string& key, const std::string& value, bool is_tombstone, std::streampos& out_offset) {
    // Prepare payload
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::system_clock::now().time_since_epoch())
                             .count();
    uint8_t tombstone = is_tombstone ? 1 : 0;
    uint32_t key_len = static_cast<uint32_t>(key.length());
    uint32_t val_len = static_cast<uint32_t>(value.length());

    // Minimize System Calls by building a one-shot buffer
    size_t total_size = sizeof(RecordHeader) + key_len + val_len;
    std::vector<char> buffer(total_size);
    
    RecordHeader* header = reinterpret_cast<RecordHeader*>(buffer.data());
    header->magic = MAGIC_BYTES;
    header->timestamp = timestamp;
    header->tombstone = tombstone;
    header->key_len = key_len;
    header->val_len = val_len;

    char* data_ptr = buffer.data() + sizeof(RecordHeader);
    std::memcpy(data_ptr, key.data(), key_len);
    std::memcpy(data_ptr + key_len, value.data(), val_len);

    const uint8_t* payload_start = reinterpret_cast<const uint8_t*>(buffer.data() + 8);
    header->checksum = CalculateCRC32(payload_start, total_size - 8);

    // Seek to end to append
    data_file_.clear();
    data_file_.seekp(0, std::ios::end);
    out_offset = data_file_.tellp();

    // Single write syscall
    data_file_.write(buffer.data(), buffer.size());
    data_file_.flush();

    return data_file_.good();
}

bool MiniDB::Put(const std::string& key, const std::string& value) {
    LockGuard<SpinLock> lock(db_mutex_);
    std::streampos offset;
    if (AppendRecord(key, value, false, offset)) {
        key_dir_[key] = offset;
        return true;
    }
    return false;
}

bool MiniDB::Delete(const std::string& key) {
    LockGuard<SpinLock> lock(db_mutex_);
    if (key_dir_.find(key) == key_dir_.end()) {
        return false; // Key not found
    }

    std::streampos offset;
    if (AppendRecord(key, "", true, offset)) {
        key_dir_.erase(key);
        return true;
    }
    
    return false;
}

bool MiniDB::Get(const std::string& key, std::string& value) {
    LockGuard<SpinLock> lock(db_mutex_);
    auto it = key_dir_.find(key);
    if (it == key_dir_.end()) {
        return false;
    }

    std::streampos offset = it->second;
    data_file_.clear();
    data_file_.seekg(offset, std::ios::beg);

    RecordHeader header;
    if (!data_file_.read(reinterpret_cast<char*>(&header), sizeof(RecordHeader))) {
        return false; // IO error
    }

    if (header.magic != MAGIC_BYTES) {
        return false; // Corrupt record
    }

    // Streamlined one-shot read for payload
    std::string payload_data;
    size_t var_len = header.key_len + header.val_len;
    payload_data.resize(var_len);
    if (!data_file_.read(&payload_data[0], var_len)) {
        return false;
    }

    // Zero-allocation CRC chain
    uint32_t crc = CalculateCRC32(reinterpret_cast<const uint8_t*>(&header.timestamp), sizeof(header.timestamp) + sizeof(header.tombstone) + sizeof(header.key_len) + sizeof(header.val_len));
    crc = CalculateCRC32(reinterpret_cast<const uint8_t*>(payload_data.data()), var_len, crc);

    if (crc != header.checksum) {
        return false; // Data corruption detected
    }

    if (header.tombstone == 1) {
        return false; // Should not happen since we remove from key_dir_, but safe check
    }

    value = payload_data.substr(header.key_len);
    return true;
}

bool MiniDB::Recover() {
    LockGuard<SpinLock> lock(db_mutex_);
    data_file_.clear();
    data_file_.seekg(0, std::ios::beg);

    key_dir_.clear();
    bool all_good = true;

    while (true) {
        std::streampos current_offset = data_file_.tellg();
        RecordHeader header;
        
        // Peek to see if EOF
        if (data_file_.peek() == EOF) {
            break;
        }

        if (!data_file_.read(reinterpret_cast<char*>(&header), sizeof(RecordHeader))) {
            break; // Reached EOF or partial header
        }

        if (header.magic != MAGIC_BYTES) {
            all_good = false;
            break; // Unknown format, stop recovery
        }

        // Handle potentially huge lengths due to corruption
        if (header.key_len > 1024 * 1024 || header.val_len > 128 * 1024 * 1024) { 
            all_good = false;
            break; // Corrupt length
        }

        // Efficient combined chunk read
        std::string payload_data;
        size_t var_len = header.key_len + header.val_len;
        payload_data.resize(var_len);
        if (!data_file_.read(&payload_data[0], var_len)) {
            all_good = false; break;
        }

        uint32_t crc = CalculateCRC32(reinterpret_cast<const uint8_t*>(&header.timestamp), sizeof(header.timestamp) + sizeof(header.tombstone) + sizeof(header.key_len) + sizeof(header.val_len));
        crc = CalculateCRC32(reinterpret_cast<const uint8_t*>(payload_data.data()), var_len, crc);

        if (crc != header.checksum) {
            all_good = false;
            break; // Checksum mismatch, stop recovery at the last valid record
        }

        std::string read_key = payload_data.substr(0, header.key_len);

        if (header.tombstone == 1) {
            key_dir_.erase(read_key);
        } else {
            key_dir_[read_key] = current_offset;
        }
    }
    
    // Clear potentially bad bits in file stream to allow further reading/writing
    data_file_.clear();
    
    return all_good;
}

bool MiniDB::Compact() {
    std::string compact_file_path = db_path_ + ".compact";
    std::unordered_map<std::string, std::streampos> temp_key_dir;

    std::ofstream compact_file(compact_file_path, std::ios::out | std::ios::binary);
    if (!compact_file.is_open()) {
        return false;
    }

    LockGuard<SpinLock> lock(db_mutex_);

    // Iterate over active keys
    for (const auto& pair : key_dir_) {
        const std::string& key = pair.first;
        std::streampos old_offset = pair.second;

        // Read from old file
        data_file_.clear();
        data_file_.seekg(old_offset, std::ios::beg);

        RecordHeader header;
        data_file_.read(reinterpret_cast<char*>(&header), sizeof(RecordHeader));
        
        // Skip key reading - we already have it! Oh wait, we need the exact bytes for the new file.
        std::string read_key;
        read_key.resize(header.key_len);
        data_file_.read(&read_key[0], header.key_len);
        
        std::string read_val;
        read_val.resize(header.val_len);
        data_file_.read(&read_val[0], header.val_len);

        // Optional: Re-calculate CRC/Timestamps or just copy raw bits.
        // It's cleaner to reuse Write/Append logic, but to keep the old timestamp,
        // we write the exact raw bits.
        
        std::streampos new_offset = compact_file.tellp();
        
        compact_file.write(reinterpret_cast<const char*>(&header), sizeof(RecordHeader));
        compact_file.write(read_key.data(), header.key_len);
        compact_file.write(read_val.data(), header.val_len);
        
        temp_key_dir[key] = new_offset;
    }

    compact_file.flush();
    compact_file.close();
    data_file_.close();

    // More robust atomic rename simulation utilizing backup tracking
    std::string backup_path = db_path_ + ".bak";
    std::remove(backup_path.c_str());
    
    // Rename current to backup
    if (std::rename(db_path_.c_str(), backup_path.c_str()) != 0) {
        return false; // Backup failed, abort
    }

    // Rename compact to current
    if (std::rename(compact_file_path.c_str(), db_path_.c_str()) != 0) {
        // Renaming failed, attempting rollback
        std::rename(backup_path.c_str(), db_path_.c_str());
        return false;
    }

    // Fully succeeded, remove safety backup
    std::remove(backup_path.c_str());

    // Reopen data file
    data_file_.open(db_path_, std::ios::in | std::ios::out | std::ios::binary | std::ios::app);
    if (!data_file_.is_open()) {
        return false; // Critical failure
    }

    key_dir_ = std::move(temp_key_dir);

    return true;
}

} // namespace minidb
