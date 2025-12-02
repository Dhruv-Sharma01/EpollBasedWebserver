cmake_minimum_required(VERSION 3.15)
project(HttpServer CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Werror -pthread)

include_directories(include)

add_executable(http_server
    src/main.cpp
    src/TcpServer.cpp
)

add_executable(benchmark
    src/benchmark.cpp
)
