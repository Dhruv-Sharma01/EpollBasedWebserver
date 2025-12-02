#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <numeric>
#include <algorithm>
#include <mutex>

// Simple blocking TCP client for benchmarking
class Client {
public:
    Client(const std::string& host, int port) : host(host), port(port) {}

    bool connectServer() {
        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd == -1) return false;

        struct sockaddr_in serv_addr;
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);

        if (inet_pton(AF_INET, host.c_str(), &serv_addr.sin_addr) <= 0) {
            close(fd);
            return false;
        }

        if (connect(fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
            close(fd);
            return false;
        }
        return true;
    }

    void sendRequest() {
        const char* req = "GET /test HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
        write(fd, req, strlen(req));
    }

    void readResponse() {
        char buffer[4096];
        while (read(fd, buffer, sizeof(buffer)) > 0) {}
    }

    void closeConnection() {
        close(fd);
    }

private:
    std::string host;
    int port;
    int fd;
};

struct Stats {
    long long requests = 0;
    long long errors = 0;
    std::vector<double> latencies; // ms
};

void worker(const std::string& host, int port, int duration, Stats& stats) {
    auto start = std::chrono::steady_clock::now();
    while (true) {
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - start).count() >= duration) {
            break;
        }

        auto reqStart = std::chrono::steady_clock::now();
        Client client(host, port);
        if (client.connectServer()) {
            client.sendRequest();
            client.readResponse();
            client.closeConnection();
            
            auto reqEnd = std::chrono::steady_clock::now();
            std::chrono::duration<double, std::milli> latency = reqEnd - reqStart;
            stats.latencies.push_back(latency.count());
            stats.requests++;
        } else {
            stats.errors++;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <host> <port> <concurrency> <duration_sec>" << std::endl;
        return 1;
    }

    std::string host = argv[1];
    int port = std::stoi(argv[2]);
    int concurrency = std::stoi(argv[3]);
    int duration = std::stoi(argv[4]);

    std::vector<std::thread> threads;
    std::vector<Stats> allStats(concurrency);

    std::cout << "Benchmarking " << host << ":" << port << " with " << concurrency << " threads for " << duration << "s..." << std::endl;

    for (int i = 0; i < concurrency; ++i) {
        threads.emplace_back(worker, host, port, duration, std::ref(allStats[i]));
    }

    for (auto& t : threads) {
        t.join();
    }

    long long totalRequests = 0;
    long long totalErrors = 0;
    std::vector<double> allLatencies;

    for (const auto& s : allStats) {
        totalRequests += s.requests;
        totalErrors += s.errors;
        allLatencies.insert(allLatencies.end(), s.latencies.begin(), s.latencies.end());
    }

    double rps = totalRequests / (double)duration;
    
    std::sort(allLatencies.begin(), allLatencies.end());
    double avgLatency = 0;
    if (!allLatencies.empty()) {
        avgLatency = std::accumulate(allLatencies.begin(), allLatencies.end(), 0.0) / allLatencies.size();
    }
    double p99 = 0;
    if (!allLatencies.empty()) {
        p99 = allLatencies[allLatencies.size() * 0.99];
    }

    std::cout << "--- Results ---" << std::endl;
    std::cout << "Total Requests: " << totalRequests << std::endl;
    std::cout << "Total Errors: " << totalErrors << std::endl;
    std::cout << "RPS: " << rps << std::endl;
    std::cout << "Avg Latency (ms): " << avgLatency << std::endl;
    std::cout << "P99 Latency (ms): " << p99 << std::endl;

    return 0;
}
