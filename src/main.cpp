#include "MiniDB.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace minidb;

void print_help() {
    std::cout << "Commands:\n"
              << "  put <key> <value>\n"
              << "  get <key>\n"
              << "  del <key>\n"
              << "  compact\n"
              << "  exit (or quit)\n";
}

int main() {
    std::cout << "=== MiniDB CLI Demonstration ===\n";
    try {
        MiniDB db("data.log");
        std::cout << "Opened database 'data.log'.\n";
        print_help();

        std::string line;
        while (true) {
            std::cout << "minidb> ";
            if (!std::getline(std::cin, line)) break;
            
            std::istringstream iss(line);
            std::string cmd;
            iss >> cmd;

            if (cmd == "exit" || cmd == "quit") break;
            if (cmd == "help") { print_help(); continue; }

            if (cmd == "put") {
                std::string key, val;
                iss >> key;
                std::getline(iss >> std::ws, val);
                if (db.Put(key, val)) std::cout << "OK\n";
                else std::cout << "Error writing to database.\n";
            } else if (cmd == "get") {
                std::string key, val;
                iss >> key;
                if (db.Get(key, val)) std::cout << val << "\n";
                else std::cout << "(not found)\n";
            } else if (cmd == "del") {
                std::string key;
                iss >> key;
                if (db.Delete(key)) std::cout << "OK\n";
                else std::cout << "(not found)\n";
            } else if (cmd == "compact") {
                if (db.Compact()) std::cout << "Compaction successful.\n";
                else std::cout << "Compaction failed.\n";
            } else if (!cmd.empty()) {
                std::cout << "Unknown command. Type 'help' for usage.\n";
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
    }
    return 0;
}
