#include "MiniDB.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include <cstdio>

using namespace minidb;

size_t get_file_size(const std::string& path) {
    std::ifstream in(path.c_str(), std::ifstream::ate | std::ifstream::binary);
    return in.tellg();
}

void test_crud() {
    std::remove("test_crud.log");
    {
        MiniDB db("test_crud.log");
        assert(db.Put("key1", "value1"));
        assert(db.Put("key2", "value2"));
        
        std::string val1;
        assert(db.Get("key1", val1));
        assert(val1 == "value1");
        
        // Update
        assert(db.Put("key1", "value1_updated"));
        assert(db.Get("key1", val1));
        assert(val1 == "value1_updated");

        // Delete
        assert(db.Delete("key1"));
        assert(!db.Get("key1", val1));
        assert(!db.Delete("key1")); // Double delete
    }
    std::cout << "[PASS] CRUD operations\n";
}

void test_recovery() {
    std::remove("test_recovery.log");
    {
        MiniDB db("test_recovery.log");
        db.Put("r1", "data1");
        db.Put("r2", "data2");
        db.Delete("r1");
    } // DB goes out of scope and is closed
    
    {
        // Recover state from file
        MiniDB db("test_recovery.log");
        std::string val;
        assert(!db.Get("r1", val)); // Was deleted
        assert(db.Get("r2", val));
        assert(val == "data2");
    }
    std::cout << "[PASS] Crash recovery & persistence\n";
}

void test_compaction() {
    std::remove("test_compact.log");
    {
        MiniDB db("test_compact.log");
        db.Put("c1", "data1");
        for(int i=0; i<100; ++i) {
            db.Put("c2", "data2_" + std::to_string(i));
        }
        db.Delete("c1");
        
        auto size_before = get_file_size("test_compact.log");
        assert(db.Compact());
        auto size_after = get_file_size("test_compact.log");
        
        assert(size_after < size_before); // Log should shrink
        
        // Data should remain intact
        std::string val;
        assert(db.Get("c2", val));
        assert(val == "data2_99");
        assert(!db.Get("c1", val)); // c1 was deleted
    }
    std::cout << "[PASS] Log Compaction\n";
}

int main() {
    try {
        test_crud();
        test_recovery();
        test_compaction();
        std::cout << "\nAll test cases passed successfully.\n";
    } catch(const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
