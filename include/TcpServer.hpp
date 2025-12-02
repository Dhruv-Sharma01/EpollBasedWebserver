#pragma once

#include "ThreadPool.hpp"
#include <string>
#include <atomic>

class TcpServer {
public:
    TcpServer(const std::string& host, int port, ThreadPool& pool);
    ~TcpServer();

    void start();

private:
    void setupSocket();
    void setNonBlocking(int fd);
    void eventLoop();
    void handleClient(int clientFd);

    std::string host;
    int port;
    int serverFd;
    int epollFd;
    ThreadPool& threadPool;
    std::atomic<bool> running;
};
