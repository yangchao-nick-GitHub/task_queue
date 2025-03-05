#include <gtest/gtest.h>
#include <chrono>
#include <atomic>
#include <iostream>
#include "task_queue.hpp"

// 测试 ComQueue 的基本功能
TEST(ComQueueTest, PushAndPop) {
    ComQueue<int> queue;
    queue.push(1);
    queue.push(2);
    queue.push(3);

    int value;
    EXPECT_TRUE(queue.tryPop(value));
    EXPECT_EQ(value, 1);
    EXPECT_TRUE(queue.tryPop(value));
    EXPECT_EQ(value, 2);
    EXPECT_TRUE(queue.tryPop(value));
    EXPECT_EQ(value, 3);
    EXPECT_FALSE(queue.tryPop(value));
}

TEST(ComQueueTest, Size) {
    ComQueue<int> queue;
    EXPECT_EQ(queue.size(), 0);
    queue.push(1);
    EXPECT_EQ(queue.size(), 1);
    queue.push(2);
    EXPECT_EQ(queue.size(), 2);
    queue.push(3);
    EXPECT_EQ(queue.size(), 3);
    int value;
    queue.tryPop(value);
    EXPECT_EQ(queue.size(), 2);
}

TEST(ComQueueTest, IsEmpty) {
    ComQueue<int> queue;
    EXPECT_TRUE(queue.isEmpty());
    queue.push(1);
    EXPECT_FALSE(queue.isEmpty());
    int value;
    queue.tryPop(value);
    EXPECT_TRUE(queue.isEmpty());
}

// 测试 WorkQueue 的基本功能
TEST(WorkQueueTest, SubmitAndExecute) {
    WorkQueue workQueue;
    std::atomic<int> counter(0);

    for (int i = 0; i < 10; ++i) {
        workQueue.submit([&counter]() {
            counter.fetch_add(1);
        });
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));  // 等待任务完成
    EXPECT_EQ(counter.load(), 10);
}

TEST(WorkQueueTest, ThreadAdjustment) {
    WorkQueue workQueue(4, 10);
    std::atomic<int> counter(0);

    for (int i = 0; i < 1000; ++i) {
        workQueue.submit([&counter]() {
            counter.fetch_add(1);
        });
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));  // 等待任务完成
    std::cout << "WorkQueueTest cur thread_num: " << workQueue.thread_num_.load() << std::endl;
    std::cout << "WorkQueueTest max thread_num: " << workQueue.history_max_thread_num_.load() << std::endl;
    std::cout << "WorkQueueTest counter: " << counter.load() << std::endl;
    EXPECT_EQ(counter.load(), 1000);
    EXPECT_LE(workQueue.thread_num_.load(), 5);
    EXPECT_GE(workQueue.thread_num_.load(), 1);
}

// 测试 WorkQueue 的性能
TEST(WorkQueueTest, Performance) {
    WorkQueue workQueue(4, 10);
    std::atomic<int> counter(0);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 1000; ++i) {
        workQueue.submit([&counter]() {
            counter.fetch_add(1);
        });
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));  // 等待任务完成
    EXPECT_EQ(counter.load(), 1000);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Time taken to process 1000 tasks: " << duration << " ms" << std::endl;
    std::cout << "cur thread num: " << workQueue.thread_num_.load() << std::endl;
    std::cout << "max thread num: " << workQueue.history_max_thread_num_.load() << std::endl;
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}