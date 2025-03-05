#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <chrono>

// 引入 TaskQueue 类定义
#include "task_queue.hpp"

// 基本功能测试
TEST(TaskQueueTest, BasicFunctionality) {
    TaskQueue<int> queue;

    // 测试 push 和 tryPop
    queue.push(42);
    int value = 0;
    EXPECT_TRUE(queue.tryPop(value));
    EXPECT_EQ(value, 42);

    // 测试 isEmpty 和 size
    EXPECT_TRUE(queue.isEmpty());
    EXPECT_EQ(queue.size(), 0);

    queue.push(10);
    queue.push(20);
    EXPECT_FALSE(queue.isEmpty());
    EXPECT_EQ(queue.size(), 2);

    // 测试多次 pop
    EXPECT_TRUE(queue.tryPop(value));
    EXPECT_EQ(value, 10);
    EXPECT_TRUE(queue.tryPop(value));
    EXPECT_EQ(value, 20);
    EXPECT_FALSE(queue.tryPop(value)); // 队列为空时应返回 false
}

// 多线程功能测试
TEST(TaskQueueTest, MultithreadedFunctionality) {
    TaskQueue<int> queue;
    const int num_threads = 10;
    const int num_tasks_per_thread = 1000;
    std::vector<std::thread> threads;

    // 启动多个线程向队列中添加任务
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&queue, i, num_tasks_per_thread]() {
            for (int j = 0; j < num_tasks_per_thread; ++j) {
                queue.push(i * num_tasks_per_thread + j);
            }
        });
    }

    // 等待所有线程完成
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    // 验证队列中的任务数量
    EXPECT_EQ(queue.size(), num_threads * num_tasks_per_thread);

    // 验证队列中的任务内容
    int count = 0;
    while (!queue.isEmpty()) {
        int value = 0;
        EXPECT_TRUE(queue.tryPop(value));
        ++count;
    }
    EXPECT_EQ(count, num_threads * num_tasks_per_thread);
}

// 性能测试
TEST(TaskQueueTest, PerformanceTest) {
    TaskQueue<int> queue;
    const int num_threads = 10;
    const int num_tasks_per_thread = 100000;
    std::vector<std::thread> producer_threads;
    std::vector<std::thread> consumer_threads;
    std::atomic<int> consumed_count{0};

    auto start_time = std::chrono::high_resolution_clock::now();

    // 生产者线程：向队列中添加任务
    for (int i = 0; i < num_threads; ++i) {
        producer_threads.emplace_back([&queue, num_tasks_per_thread]() {
            for (int j = 0; j < num_tasks_per_thread; ++j) {
                queue.push(j);
            }
        });
    }

    // 消费者线程：从队列中取出任务
    for (int i = 0; i < num_threads; ++i) {
        consumer_threads.emplace_back([&queue, &consumed_count]() {
            int value = 0;
            while (true) {
                if (queue.tryPop(value)) {
                    ++consumed_count;
                } else if (queue.isEmpty()) {
                    break;
                }
            }
        });
    }

    // 等待生产者线程完成
    for (auto& thread : producer_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    // 等待消费者线程完成
    for (auto& thread : consumer_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    // 验证消费的任务数量
    EXPECT_EQ(consumed_count.load(), num_threads * num_tasks_per_thread);

    // 输出性能指标
    std::cout << "Processed " << consumed_count.load() << " tasks in " << duration << " ms." << std::endl;
}

int main(int argc, char** argv) {
    // 初始化 Google Test 框架
    ::testing::InitGoogleTest(&argc, argv);

    // 运行所有测试用例
    return RUN_ALL_TESTS();
}