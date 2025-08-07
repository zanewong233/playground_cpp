#include <gtest/gtest.h>

#include <numeric>

#include "playground/threading/lockfree_stack.hpp"

using namespace playground;

// 单线程测试：基础功能验证
TEST(LockfreeStackTest, SingleThread) {
  LockfreeStack<int> stack;
  EXPECT_EQ(stack.Pop(), nullptr);

  stack.Push(1);
  stack.Push(2);
  stack.Push(3);

  auto p3 = stack.Pop();
  ASSERT_TRUE(p3);
  EXPECT_EQ(*p3, 3);
  auto p2 = stack.Pop();
  ASSERT_TRUE(p2);
  EXPECT_EQ(*p2, 2);
  auto p1 = stack.Pop();
  ASSERT_TRUE(p1);
  EXPECT_EQ(*p1, 1);

  EXPECT_EQ(stack.Pop(), nullptr);
}

// 多线程生产者/消费者测试：保证线程安全性与无死锁
TEST(LockfreeStackTest, MultiThreadProducerConsumer) {
  LockfreeStack<int> stack;
  const int num_producers = 4;
  const int num_consumers = 4;
  const int items_per_producer = 10000;

  std::atomic<int> produced_count{0};
  std::atomic<int> consumed_count{0};

  std::vector<long long> produced_sum_vec(num_producers, 0);
  std::vector<long long> consumed_sum_vec(num_consumers, 0);

  // 生产者线程：推入 items_per_producer 个元素
  auto producer = [&](int id, long long* sum) {
    for (int i = 0; i < items_per_producer; ++i) {
      int val = id * items_per_producer + i;
      *sum += val;
      stack.Push(val);
      produced_count.fetch_add(1, std::memory_order_relaxed);
    }
  };

  // 消费者线程：一直 Pop，直到总消费数达到预期
  auto consumer = [&](long long* sum) {
    while (true) {
      auto p = stack.Pop();
      if (p) {
        (*sum) += (*p);
        consumed_count.fetch_add(1, std::memory_order_relaxed);
      } else {
        if (consumed_count.load(std::memory_order_relaxed) <
            num_producers * items_per_producer)
          continue;
        else
          break;
      }
    }
  };

  std::vector<std::thread> producers;
  std::vector<std::thread> consumers;
  for (int i = 0; i < num_producers; ++i) {
    producers.emplace_back(producer, i, &produced_sum_vec[i]);
  }
  for (int i = 0; i < num_consumers; ++i) {
    consumers.emplace_back(consumer, &consumed_sum_vec[i]);
  }

  for (auto& t : producers) t.join();
  for (auto& t : consumers) t.join();

  EXPECT_EQ(produced_count.load(), num_producers * items_per_producer);
  EXPECT_EQ(consumed_count.load(), produced_count.load());

  auto produced_sum =
      std::accumulate(produced_sum_vec.begin(), produced_sum_vec.end(), 0);
  auto consumed_sum =
      std::accumulate(consumed_sum_vec.begin(), consumed_sum_vec.end(), 0);
  EXPECT_EQ(produced_sum, consumed_sum);
}

// 混合测试：交错 Push/Pop 操作
TEST(LockfreeStackTest, MixedPushPop) {
  LockfreeStack<int> stack;
  const int threads = 4;
  const int ops_per_thread = 5000;
  std::atomic<int> net_count{0};

  auto worker = [&](int id) {
    for (int i = 0; i < ops_per_thread; ++i) {
      if ((i + id) % 2 == 0) {
        stack.Push(i);
        net_count.fetch_add(1, std::memory_order_relaxed);
      } else {
        auto p = stack.Pop();
        if (p) net_count.fetch_sub(1, std::memory_order_relaxed);
      }
    }
  };

  std::vector<std::thread> ws;
  for (int i = 0; i < threads; ++i) ws.emplace_back(worker, i);
  for (auto& t : ws) t.join();

  int count = 0;
  while (auto p = stack.Pop()) ++count;
  EXPECT_EQ(count, net_count.load());
}
