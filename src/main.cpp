#include "TcpServer.hpp"
#include "ThreadPool.hpp"
#include <iostream>

int main() {
    try {
        // Initialize ThreadPool with hardware concurrency
        ThreadPool pool;
        
        // Initialize Server on port 8080
        TcpServer server("0.0.0.0", 8080, pool);
        
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
