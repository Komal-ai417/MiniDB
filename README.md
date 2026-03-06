# MiniDB: A Log-Structured Key-Value Store

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-11-blue.svg" alt="C++ Version">
  <img src="https://img.shields.io/badge/STL-Standard_Library-orange.svg" alt="C++ STL">
  <img src="https://img.shields.io/badge/Dependencies-Zero-success.svg" alt="Zero Dependencies">
  <img src="https://img.shields.io/badge/License-MIT-green.svg" alt="License">
</p>

MiniDB is a lightweight, persistent, NoSQL key-value store engineered from scratch in C++. By utilizing a Log-Structured Merge (LSM) approach alongside an in-memory Hash Index, MiniDB achieves unparalleled **O(1) read and write performance** suitable for millions of records. 

Built entirely using the **C++ Standard Library (STL) and File I/O**, it operates with absolute zero external dependencies, establishing a highly robust, dependency-less architectural design pattern.

---

## ⚡ Key Engineering Achievements

- ✔️ **O(1) Latency**: Engineered a persistent NoSQL database from scratch using an append-only, log-structured merge (LSM) approach, guaranteeing constant-time O(1) write performance.
- ✔️ **Optimized Retrieval**: Optimized read queries using an in-memory Hash Index, successfully bypassing slow random-access disk seeks to achieve O(1) retrieval time for millions of records.
- ✔️ **Crash-Safe Compaction**: Implemented an automated background compaction process to reclaim disk space from old versions and deleted records without data loss during intermittent power or system failures.
- ✔️ **Custom Binary Protocol**: Designed a rigorous binary protocol equipped with magic bytes (`MDB1`) and CRC32 checksums, immediately detecting file corruption or incomplete writes to ensure stringent data integrity.

---

## 🏗 Storage Architecture

### Append-Only Log Mechanics
Unlike traditional B-Trees that force slow, random, in-place disk updates, MiniDB treats the core database file natively as an append-only log. This completely eliminates random disk I/O penalties during inserts and deletes.

### Binary Protocol
Every key-value pair is tightly packed into the log using a custom layout. 
```text
+-------------------+-------------------+-------------------+
| Magic Bytes (4B)  | CRC32 (4B)        | Timestamp (8B)    |
+-------------------+-------------------+-------------------+
| Tombstone (1B)    | Key Length (4B)   | Value Length (4B) |
+-------------------+-------------------+-------------------+
| Key (Variable Length)                 |
+---------------------------------------+
| Value (Variable Length)               |
+---------------------------------------+
```

### Crash Recovery Mechanism
On system boot, MiniDB scans the log file linearly. It independently calculates the CRC32 of every record and checks it against the stored checksum. In the event of an abrupt system crash mid-write, the final corrupted record is seamlessly identified and naturally truncated, avoiding complete system failure and halting recovery at the last known-good state.

---

## 🚀 Getting Started

### Build Instructions

MiniDB is self-contained. It compiles instantly utilizing any modern C++ toolchain (GCC, Clang, MSVC).

```bash
# Clone the repository
git clone https://github.com/Komal-ai417/minidb.git
cd minidb

# Compile the testing suite (Custom dependency-free tests)
g++ -std=c++11 -I./include src/MiniDB.cpp tests/test_minidb.cpp -o test_minidb

# Run the strict validation tests
./test_minidb

# Compile the Command Line Interface
g++ -std=c++11 -I./include src/MiniDB.cpp src/main.cpp -o minidb_cli

# Launch the interactive CLI
./minidb_cli
```

### Interactive CLI

```text
=== MiniDB CLI Demonstration ===
Opened database 'data.log'.
Commands:
  put <key> <value>
  get <key>
  del <key>
  compact
  exit (or quit)

minidb> put server_ip 192.168.1.100
OK
minidb> get server_ip
192.168.1.100
minidb> del server_ip
OK
minidb> get server_ip
(not found)
minidb> compact
Compaction successful.
minidb> exit
```

---

## 💻 Native C++ API

If you need a lightweight NoSQL persistent store for your C++ system, you can directly embed `MiniDB` seamlessly:

```cpp
#include "MiniDB.h"
#include <iostream>

using namespace minidb;

int main() {
    // Zero config - opens or automatically establishes a new log
    MiniDB db("system_metrics.log");

    // Persist data in O(1) Time
    if (db.Put("cpu_temp", "45C")) {
        std::cout << "Metric recorded instantly!\n";
    }

    // Retrieve data in O(1) Time
    std::string value;
    if (db.Get("cpu_temp", value)) {
        std::cout << "Retrieved: " << value << "\n";
    }

    // O(1) Marker Deletions
    db.Delete("cpu_temp");

    // Securely trigger crash-safe garbage collection
    db.Compact();

    return 0;
}
```

---

## 📝 License
This project is licensed under the MIT License.
