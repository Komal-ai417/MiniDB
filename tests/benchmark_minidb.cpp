#include "MiniDB.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <string>
#include <cstdio>
#include <atomic>

using namespace minidb;

void stress_test_sequential() {
    std::remove("stress_seq.log");
    {
        MiniDB db("stress_seq.log");
        const int NUM_OPS = 100000; // 100k operations
        
        std::cout << "--- Sequential Stress Test ---\n";
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < NUM_OPS; ++i) {
            db.Put("key" + std::to_string(i), "value_value_value_value_value_" + std::to_string(i));
        }
        auto end_put = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < NUM_OPS; ++i) {
            db.Get("key" + std::to_string(i));
        }
        auto end_get = std::chrono::high_resolution_clock::now();

        db.Compact();
        auto end_compact = std::chrono::high_resolution_clock::now();

        std::cout << "Puts (" << NUM_OPS << "): " << std::chrono::duration_cast<std::chrono::milliseconds>(end_put - start).count() << " ms\n";
        std::cout << "Gets (" << NUM_OPS << "): " << std::chrono::duration_cast<std::chrono::milliseconds>(end_get - end_put).count() << " ms\n";
        std::cout << "Compact: " << std::chrono::duration_cast<std::chrono::milliseconds>(end_compact - end_get).count() << " ms\n\n";
    }
}

void stress_test_concurrent() {
    std::remove("stress_conc.log");
    {
        MiniDB db("stress_conc.log");
        const int NUM_THREADS = 8;
        const int OPS_PER_THREAD = 10000;
        
        std::cout << "--- Concurrent Stress Test ---\n";
        std::cout << "Running " << NUM_THREADS << " threads, " << OPS_PER_THREAD << " puts/gets per thread...\n";
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        std::atomic<int> failed_puts{0};
        std::atomic<int> failed_gets{0};

        for (int t = 0; t < NUM_THREADS; ++t) {
            threads.emplace_back([&db, t, &failed_puts, &failed_gets, OPS_PER_THREAD]() {
                for (int i = 0; i < OPS_PER_THREAD; ++i) {
                    std::string key = "t" + std::to_string(t) + "_k" + std::to_string(i);
                    std::string val = "concurrent_data_" + std::to_string(i);
                    if (!db.Put(key, val)) failed_puts++;
                }
                for (int i = 0; i < OPS_PER_THREAD; ++i) {
                    std::string key = "t" + std::to_string(t) + "_k" + std::to_string(i);
                    if (!db.Get(key).has_value()) failed_gets++;
                }
            });
        }

        for (auto& th : threads) th.join();
        auto end = std::chrono::high_resolution_clock::now();
        
        std::cout << "Time taken: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";
        std::cout << "Failed Puts: " << failed_puts << "\n";
        std::cout << "Failed Gets: " << failed_gets << "\n";
    }
}

int main() {
    std::cout << "Starting MiniDB Stress Tests...\n\n";
    stress_test_sequential();
    stress_test_concurrent();
    std::cout << "\nStress tests completed.\n";
    return 0;
}
