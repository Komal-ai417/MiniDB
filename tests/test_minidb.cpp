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
        
        auto val1 = db.Get("key1");
        assert(val1.has_value());
        assert(val1.value() == "value1");
        
        // Update
        assert(db.Put("key1", "value1_updated"));
        val1 = db.Get("key1");
        assert(val1.has_value());
        assert(val1.value() == "value1_updated");

        // Delete
        assert(db.Delete("key1"));
        assert(!db.Get("key1").has_value());
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
        assert(!db.Get("r1").has_value()); // Was deleted
        auto val2 = db.Get("r2");
        assert(val2.has_value());
        assert(val2.value() == "data2");
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
        auto val = db.Get("c2");
        assert(val.has_value());
        assert(val.value() == "data2_99");
        assert(!db.Get("c1").has_value()); // c1 was deleted
    }
    std::cout << "[PASS] Log Compaction\n";
}

void test_variable_data() {
    std::remove("test_variable.log");
    {
        MiniDB db("test_variable.log");
        
        // Empty key, empty value
        assert(db.Put("", ""));
        auto v1 = db.Get("");
        assert(v1.has_value() && v1.value() == "");

        // Empty key, normal value
        assert(db.Put("", "value"));
        v1 = db.Get("");
        assert(v1.has_value() && v1.value() == "value");

        // Normal key, empty value
        assert(db.Put("empty_val", ""));
        auto v2 = db.Get("empty_val");
        assert(v2.has_value() && v2.value() == "");

        // Null bytes in strings
        std::string null_str("hello\0world", 11);
        assert(db.Put("null_str", null_str));
        auto v3 = db.Get("null_str");
        assert(v3.has_value() && v3.value() == null_str);
        
        // Very large value (1 MB)
        std::string large_val(1024 * 1024, 'x');
        assert(db.Put("large", large_val));
        auto v4 = db.Get("large");
        assert(v4.has_value() && v4.value() == large_val);
    }
    std::cout << "[PASS] Variable data sizes & types\n";
}

void test_edge_cases() {
    std::remove("test_edge.log");
    {
        MiniDB db("test_edge.log");
        
        // Get non-existent
        assert(!db.Get("non_existent").has_value());

        // Delete non-existent
        assert(!db.Delete("non_existent"));

        // Put then delete then get
        assert(db.Put("to_delete", "val"));
        assert(db.Delete("to_delete"));
        assert(!db.Get("to_delete").has_value());

        // Many deletes (compact should handle 100% tombstones)
        for (int i = 0; i < 100; ++i) {
            db.Put("k" + std::to_string(i), "v");
            db.Delete("k" + std::to_string(i));
        }
        assert(db.Compact());
        auto size = get_file_size("test_edge.log");
        assert(size == 0); // All keys deleted, file should be completely empty
    }
    
    // Simulate corrupt record/file
    {
        std::ofstream out("test_corrupt.log", std::ios::binary);
        out << "random garbage data";
        out.close();
        
        // Recover should fail gracefully on garbage
        MiniDB db("test_corrupt.log");
        assert(!db.Get("any").has_value());
    }
    std::remove("test_corrupt.log");

    std::cout << "[PASS] Edge cases & corruption handling\n";
}

int main() {
    try {
        test_crud();
        test_recovery();
        test_compaction();
        test_variable_data();
        test_edge_cases();
        std::cout << "\nAll test cases passed successfully.\n";
    } catch(const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
