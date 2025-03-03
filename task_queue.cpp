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
private:
    std::mutex mtx_;
    std::deque<T> tq_;
};


class ThreadPool {
public:
    ThreadPool();
    explicit ThreadPool(uint32_t thread_num);
    ~ThreadPool();
    void init();
    void submit(std::function<void()> task);
    void workMonitor();
    void addWorker();

private:
    uint32_t thread_num_;
    std::vector<std::thread> threads_;
    TaskQueue<std::function<void()>> task_quque_;
    std::mutex thread_mutex_;
    std::condition_variable thread_cond_;
    bool montior_stop_ {false};
};

ThreadPool::ThreadPool()
{
    thread_num_ = 5;
    init();
}

ThreadPool::ThreadPool(uint32_t thread_num):thread_num_(thread_num > 10 ? 10 : thread_num)
{
    init();
}

ThreadPool::~ThreadPool()
{   
    montior_stop_ = true;
    thread_cond_.notify_all();
    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void ThreadPool::submit(std::function<void()> task)
{
    if (montior_stop_) {
        throw std::runtime_error("Thread pool is stopping");
    }
    std::lock_guard<std::mutex> lock(thread_mutex_);
    task_quque_.push(task);
    thread_cond_.notify_one();
}

void ThreadPool::workMonitor()
{
    while (!montior_stop_) {
        std::function<void()> task;
        std::unique_lock<std::mutex> lock(thread_mutex_);
        thread_cond_.wait_for(lock, std::chrono::seconds(1), [this] {
            return montior_stop_ || !task_quque_.isEmpty();
        });

        if (!task_quque_.tryPop(task)) {
            continue;
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

void ThreadPool::init()
{
    for (uint32_t i = 0; i < thread_num_; i++) {
        addWorker();
    }
}

void ThreadPool::addWorker()
{
    threads_.emplace_back(&ThreadPool::workMonitor, this);
}


int main()
{
    ThreadPool pool(4);
    pool.submit([]() {
        fprintf(stderr, "task1 Hello world\n");    
    });
    pool.submit([]() {
        fprintf(stderr, "task2 Hello world\n");    
    });
    pool.submit([]() {
        fprintf(stderr, "task3 Hello world\n");    
    });

    std::this_thread::sleep_for(std::chrono::seconds(10));

    return 0;
}