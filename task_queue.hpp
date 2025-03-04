#include <iostream>
#include <vector>
#include <queue>
#include <deque>
#include <mutex>
#include <string>
#include <functional>
#include <thread>
#include <condition_variable>
#include <algorithm>
#include <unordered_map>
#include <atomic>

template<typename T>
class TaskQueue {
public:
    TaskQueue() = default;
    TaskQueue(const TaskQueue&) = delete;
    TaskQueue(TaskQueue&&) = default;

    void push(const T& task)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        tq_.emplace_back(task);
    }

    bool tryPop(T& task)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (!tq_.empty()) {
            task = std::move(tq_.front());
            tq_.pop_front();
            return true;
        }
        return false;
    }

    bool isEmpty()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return tq_.empty();
    }

    size_t size()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return tq_.size();
    }

private:
    std::mutex mtx_;
    std::deque<T> tq_;
};

class ThreadPool {
public:
    explicit ThreadPool(uint32_t min_thread_num = 4, uint32_t max_thread_num = 10)
    {
        min_thread_num_ = std::max(1U, min_thread_num);
        max_thread_num_ = std::max(min_thread_num_, max_thread_num);
        thread_num_.store(min_thread_num);
        init();
    }

    ~ThreadPool()
    {
        is_running = false;
        thread_cond_.notify_all();
        for (auto& thread : threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

    void submit(std::function<void()> task)
    {
        std::lock_guard<std::mutex> lock(thread_mutex_);
        task_quque_.push(task);
        adjustThreadNum();
        thread_cond_.notify_one();
    }

    void workMonitor()
    {
        while (is_running) {
            if (task_quque_.isEmpty() && thread_num_.load() > min_thread_num_) {
                thread_num_.fetch_sub(1);
                return;
            }
            if (!is_running) {
                break;
            }
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(thread_mutex_);
                thread_cond_.wait_for(lock, std::chrono::seconds(1), [this] {
                    return !is_running || !task_quque_.isEmpty();
                });

                if (!task_quque_.tryPop(task)) {
                    continue;
                }
            }

            try {
                task();
            } catch (const std::exception& e) {
                std::cerr << "Thread pool error: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Thread pool error: unknown exception" << std::endl;
            }
        }
    }

    void init()
    {
        for (uint32_t i = 0; i < thread_num_; i++) {
            addWorker();
        }
    }

    void addWorker()
    {
        threads_.emplace_back(&ThreadPool::workMonitor, this);
        thread_num_.fetch_add(1);
    }

private:
    std::atomic<uint32_t> thread_num_;
    uint32_t min_thread_num_ {1};
    uint32_t max_thread_num_ {10};
    std::vector<std::thread> threads_;
    TaskQueue<std::function<void()>> task_quque_;
    std::mutex thread_mutex_;
    std::condition_variable thread_cond_;
    bool is_running {true};

    void adjustThreadNum()
    {
        if (task_quque_.size() > thread_num_.load() * 2 && thread_num_.load() < max_thread_num_) {
            addWorker();
        }
    }
};

