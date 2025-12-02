# High-Performance C++20 HTTP Server

A high-throughput, low-latency HTTP/1.1 web server built from scratch using C++20 and raw Linux syscalls.

## Features
- **Architecture**: Reactor Pattern with Non-blocking I/O (Epoll Edge-Triggered).
- **Concurrency**: Custom Thread Pool using `std::jthread` and `std::stop_token`.
- **Performance**: Zero-copy parsing (using `std::span` and `std::string_view`), lock-free-ish design (OneShot Epoll).
- **No Dependencies**: Pure C++20 and Linux headers (`<sys/epoll.h>`, `<sys/socket.h>`).

## Build & Run

### Prerequisites
- Linux OS
- CMake >= 3.15
- C++20 compliant compiler (GCC 10+ or Clang 10+)

### Compilation
```bash
mkdir build
cd build
cmake ..
make
```

### Running the Server
```bash
./build/http_server
```
The server listens on `0.0.0.0:8080`.

## Benchmarking

This project includes a custom high-performance benchmarking tool written in C++.

### Running Benchmarks
```bash
# Run the python runner (automatically compiles and runs benchmarks)
python3 benchmark_runner.py
```

### Performance Results
Tested on local loopback (127.0.0.1):

| Concurrency | RPS | Avg Latency (ms) |
|-------------|-----|------------------|
| 10          | ~50k| 0.20             |
| 100         | ~58k| 1.72             |
| 500         | ~57k| 8.78             |

*Note: Results may vary based on hardware.*

## Project Structure
- `include/TcpServer.hpp`: Core server logic (Epoll loop).
- `include/ThreadPool.hpp`: Worker thread pool.
- `include/HttpUtils.hpp`: HTTP parsing and response generation.
- `src/benchmark.cpp`: Load generator tool.
