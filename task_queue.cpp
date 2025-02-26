#include <iostream>
#include <vector>
#include <queue>
#include <mutex>
#include <functional>
#include <thread>


class ThreadPool {
public:
    ThreadPool(uint32_t thread_num):thread_num_(thread_num) { init(); }
    ~ThreadPool();
    void init();
    void submit(std::function<void()> task);
    void threadLoop();
private:
    uint32_t thread_num_;
    std::vector<std::thread> threads_;
    std::queue<std::function<void()>> tasks_;
    std::mutex tasks_mutex_;
};

ThreadPool::~ThreadPool()
{
    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void ThreadPool::submit(std::function<void()> task)
{
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    tasks_.push(task);
}


void ThreadPool::threadLoop()
{
    while (true) {
        std::unique_lock<std::mutex> lock(tasks_mutex_);
        if (!tasks_.empty()) {
            auto task = tasks_.front();
            tasks_.pop();
            task();
        }
        lock.unlock();
    }

}

void ThreadPool::init()
{
    for (int i = 0; i < thread_num_; i++) {
        threads_.emplace_back(&ThreadPool::threadLoop, this);
    }
}


int main()
{
    ThreadPool pool(4);
    pool.submit([]() {
        fprintf(stderr, "task1 Hello from thread id:%lld\n", std::this_thread::get_id());    
    });
    pool.submit([]() {
        fprintf(stderr, "task2 Hello from thread id:%lld\n", std::this_thread::get_id());
    });
        pool.submit([]() {
        fprintf(stderr, "task3 Hello from thread id:%lld\n", std::this_thread::get_id());    
    });
    pool.submit([]() {
        fprintf(stderr, "task4 Hello from thread id:%lld\n", std::this_thread::get_id());
    });
        pool.submit([]() {
        fprintf(stderr, "task5 Hello from thread id:%lld\n", std::this_thread::get_id());    
    });
    pool.submit([]() {
        fprintf(stderr, "task6 Hello from thread id:%lld\n", std::this_thread::get_id());
    });

    std::this_thread::sleep_for(std::chrono::seconds(10));



    return 0;
}