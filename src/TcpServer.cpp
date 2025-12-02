#include "TcpServer.hpp"
#include "HttpUtils.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <vector>
#include <array>

#define MAX_EVENTS 1024
#define BUFFER_SIZE 4096

TcpServer::TcpServer(const std::string& host, int port, ThreadPool& pool)
    : host(host), port(port), serverFd(-1), epollFd(-1), threadPool(pool), running(false) {}

TcpServer::~TcpServer() {
    running = false;
    if (serverFd != -1) close(serverFd);
    if (epollFd != -1) close(epollFd);
}

void TcpServer::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        return;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL");
    }
}

void TcpServer::setupSocket() {
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Bind to all interfaces
    address.sin_port = htons(port);

    if (bind(serverFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    setNonBlocking(serverFd);

    if (listen(serverFd, SOMAXCONN) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    epollFd = epoll_create1(0);
    if (epollFd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET; // Edge-triggered
    event.data.fd = serverFd;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serverFd, &event) == -1) {
        perror("epoll_ctl: serverFd");
        exit(EXIT_FAILURE);
    }
}

void TcpServer::start() {
    setupSocket();
    running = true;
    std::cout << "Server listening on port " << port << "..." << std::endl;
    eventLoop();
}

void TcpServer::eventLoop() {
    std::array<struct epoll_event, MAX_EVENTS> events;

    while (running) {
        int n = epoll_wait(epollFd, events.data(), MAX_EVENTS, -1);
        if (n == -1) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;

            if (fd == serverFd) {
                // Handle new connections
                while (true) {
                    struct sockaddr_in clientAddr;
                    socklen_t clientLen = sizeof(clientAddr);
                    int clientFd = accept(serverFd, (struct sockaddr*)&clientAddr, &clientLen);

                    if (clientFd == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            // We have processed all incoming connections
                            break;
                        } else {
                            perror("accept");
                            break;
                        }
                    }

                    setNonBlocking(clientFd);

                    // Add to epoll with ONESHOT to ensure only one thread handles it at a time
                    struct epoll_event event;
                    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
                    event.data.fd = clientFd;
                    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &event) == -1) {
                        perror("epoll_ctl: clientFd");
                        close(clientFd);
                    }
                }
            } else {
                // Handle data from client
                // Remove from epoll or just rely on ONESHOT.
                // Since we used ONESHOT, we don't strictly need to remove it, 
                // but we must re-arm it if we want to read again later.
                // However, our current design is Read -> Write -> Close.
                
                // We pass the file descriptor to the thread pool
                threadPool.enqueue([this, fd]() {
                    this->handleClient(fd);
                });
            }
        }
    }
}

void TcpServer::handleClient(int clientFd) {
    std::vector<char> buffer(BUFFER_SIZE);
    std::string requestData;

    while (true) {
        ssize_t bytesRead = read(clientFd, buffer.data(), BUFFER_SIZE);
        if (bytesRead == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No more data to read right now
                break;
            } else {
                perror("read");
                close(clientFd);
                return;
            }
        } else if (bytesRead == 0) {
            // Client closed connection
            close(clientFd);
            return;
        }

        requestData.append(buffer.data(), bytesRead);
        
        // Simple check for end of headers
        if (requestData.find("\r\n\r\n") != std::string::npos) {
            break;
        }
    }

    if (requestData.empty()) {
        close(clientFd);
        return;
    }

    // Parse Request
    HttpRequest req = HttpUtils::parseRequest(std::span<const char>(requestData.data(), requestData.size()));
    
    // Generate Response
    HttpResponse res;
    res.statusCode = 200;
    res.body = "<html><body><h1>Hello from C++20 High-Performance Server!</h1><p>Requested: " + req.path + "</p></body></html>";
    res.headers["Content-Type"] = "text/html";

    std::string responseStr = res.toString();

    // Write Response
    ssize_t bytesWritten = write(clientFd, responseStr.data(), responseStr.size());
    if (bytesWritten == -1) {
        perror("write");
    }

    // Close connection (Short-lived connection for this demo)
    // To support Keep-Alive:
    // 1. Check "Connection: keep-alive" header
    // 2. If keep-alive, do NOT close.
    // 3. Re-arm epoll:
    //    struct epoll_event event;
    //    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    //    event.data.fd = clientFd;
    //    epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &event);
    
    close(clientFd);
}
