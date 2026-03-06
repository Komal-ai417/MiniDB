# MiniDB: A Log-Structured Key-Value Store

<p align="center">
  <img src="https://img.shields.io/github/actions/workflow/status/Komal-ai417/minidb/ci.yml?branch=main" alt="Build Status">
  <img src="https://img.shields.io/badge/Language-C%2B%2B11-blue.svg" alt="C++ Version">
  <img src="https://img.shields.io/badge/Architecture-LSM_Tree-orange.svg" alt="Architecture">
  <img src="https://img.shields.io/badge/Dependencies-Zero-success.svg" alt="Zero Dependencies">
  <img src="https://img.shields.io/badge/License-MIT-green.svg" alt="License">
</p>

MiniDB is a highly-optimized, lightweight, and persistent NoSQL key-value store engineered from scratch in **C++**. 
By embracing a Log-Structured Merge (LSM) architectural design pattern and an in-memory Hash Index, MiniDB achieves unparalleled **O(1) read and write performance**, effortlessly scaling to millions of records. 

Built exclusively utilizing the **C++ Standard Library (STL) and POSIX-compliant File I/O**, it operates with **absolute zero external dependencies**, establishing a robust, easily-embeddable database engine.

---

## ⚡ Key Engineering Achievements

- **Zero-Allocation I/O Pipeline**: Architected a one-shot batched File I/O system minimizing expensive kernel context switches. Validated with a Zero-Allocation CRC32 Lookup Table (LUT) hashing strategy utilizing explicit memory addresses, avoiding all heap/`std::vector` allocations per query.
- **O(1) Latency Guarantee**: Specifically designed around an append-only log paradigm, preventing random-disk seeks. Every `Put` or `Delete` operation is executed as a constant-time sequential disk write.
- **Optimized Hash Retrieval**: Bypassed traditional disk indexing by maintaining a live Hash Directory mapped directly to exact disk offsets, achieving true O(1) data retrieval times.
- **Resilient Crash Recovery**: Automated background file scanning with byte-level precision. In the event of system power failure, corrupted blocks or torn-writes are dynamically identified via `MDB1` Magic Bytes and truncated automatically without compromising the unified dataset.
- **Live Compaction Garbage Collection**: Implemented a transactional space-reclamation engine that asynchronously compacts the active database, gracefully purging outdated keys and tombstones (deleted records) ensuring a minimal disk footprint.

---

## 🏗 Deep-Dive: Storage Architecture

### Append-Only Log Mechanics vs. B-Trees
Traditional relational databases (B-Trees) utilize in-place updates, which critically degrade performance on rotational or slow media due to heavy random disk I/O penalties. **MiniDB treats the database natively as an infinite append-only log.** All operations—whether new insertions or overriding updates—are simply appended sequentially to the very end of the file.

### Custom Binary Serialization Protocol
Every stored attribute is strictly packed byte-by-byte into the file stream utilizing a custom struct layout to ensure rigid alignment and data security. 

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
*(Each complete transaction mathematically hashes the core payload against the stored CRC32 header to mathematically prove structural integrity upon read).*

---

## 🚀 Getting Started

### Build Instructions

As a dependency-less engine, MiniDB compiles instantly on any modern system natively.

```bash
# 1. Clone the repository
git clone https://github.com/Komal-ai417/minidb.git
cd minidb

# 2. Compile the rigorous Test Suite (Custom dependency-free framework)
g++ -std=c++11 -I./include src/MiniDB.cpp tests/test_minidb.cpp -o test_minidb

# 3. Execute the strict validation tests
./test_minidb
#> [PASS] CRUD operations
#> [PASS] Crash recovery & persistence
#> [PASS] Log Compaction

# 4. Compile the Command Line Interface (CLI)
g++ -std=c++11 -I./include src/MiniDB.cpp src/main.cpp -o minidb_cli

# 5. Launch the interactive Database Terminal
./minidb_cli
```

### Interactive CLI Walkthrough

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

## 💻 Embed into your C++ Project

If you need a blazing-fast, lightweight NoSQL store dynamically bolted into your C++ application, initializing `MiniDB` takes three lines of code:

```cpp
#include "MiniDB.h"
#include <iostream>

using namespace minidb;

int main() {
    // Zero config logic - opens or automatically establishes a new system-local log
    MiniDB db("system_metrics.log");

    // Perform O(1) Sequential Disk Writes
    if (db.Put("cpu_temp", "45C")) {
        std::cout << "Metric recorded instantly!\n";
    }

    // Perform O(1) Memory-Mapped Offset Reads
    std::string value;
    if (db.Get("cpu_temp", value)) {
        std::cout << "Retrieved: " << value << "\n";
    }

    // Perform O(1) Marker Deletions
    db.Delete("cpu_temp");

    // Securely trigger crash-safe garbage collection
    db.Compact();

    return 0;
}
```

---

## 🤝 Contributions
Contributions, issues, and feature requests are welcome! Feel free to check the [issues page](https://github.com/Komal-ai417/minidb/issues).

## 📝 License
This project is licensed strictly under the [MIT License](LICENSE).
