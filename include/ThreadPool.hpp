#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <concepts>

class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads = std::thread::hardware_concurrency()) {
        for (size_t i = 0; i < numThreads; ++i) {
            workers.emplace_back([this](std::stop_token stopToken) {
                while (!stopToken.stop_requested()) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        condition.wait(lock, [this, &stopToken] {
                            return stopToken.stop_requested() || !tasks.empty();
                        });

                        if (stopToken.stop_requested() && tasks.empty()) {
                            return;
                        }

                        if (tasks.empty()) {
                            continue;
                        }

                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    ~ThreadPool() {
        // jthread automatically requests stop and joins on destruction
        // But we need to wake up threads waiting on the condition variable
        stop();
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            // We don't have a separate 'stop' flag because we rely on jthread's stop_token
            // However, to wake up the condition variable we might need to notify.
            // Since jthread stop_request doesn't automatically notify the CV, we do it here.
        }
        // Request stop for all threads (jthread destructor does this, but we want to wake them up)
        for(auto& worker : workers) {
            worker.request_stop();
        }
        condition.notify_all();
    }

    template<class F>
    void enqueue(F&& f) {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            tasks.emplace(std::forward<F>(f));
        }
        condition.notify_one();
    }

private:
    std::vector<std::jthread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
};
