#include <gtest/gtest.h>

#include <future>
#include <numeric>

#include "playground/threading/lockfree_stack_hazard_pointer.hpp"
#include "playground/threading/lockfree_stack_reference_counting.hpp"
#include "playground/threading/lockfree_stack_shared_ptr.hpp"
#include "playground/threading/lockfree_statck_delete_list.hpp"

using namespace playground;

using Implementations = ::testing::Types<
    LockfreeStackDeleteList<int>, LockfreeStackHazardPointer<int>,
    LockfreeStackSharedPtr<int>, LockfreeStackReferenceCounting<int>>;

template<typename StackT>
class LockfreeStackTest : public ::testing::Test {
 protected:
  StackT stack_;
};
TYPED_TEST_SUITE(LockfreeStackTest, Implementations);

// 单线程测试：基础功能验证
TYPED_TEST(LockfreeStackTest, SingleThread) {
  EXPECT_EQ(this->stack_.Pop(), nullptr);

  this->stack_.Push(1);
  this->stack_.Push(2);
  this->stack_.Push(3);

  auto p3 = this->stack_.Pop();
  ASSERT_TRUE(p3);
  EXPECT_EQ(*p3, 3);
  auto p2 = this->stack_.Pop();
  ASSERT_TRUE(p2);
  EXPECT_EQ(*p2, 2);
  auto p1 = this->stack_.Pop();
  ASSERT_TRUE(p1);
  EXPECT_EQ(*p1, 1);

  EXPECT_EQ(this->stack_.Pop(), nullptr);
}

// 多线程生产者/消费者测试：保证线程安全性与无死锁
TYPED_TEST(LockfreeStackTest, MultiThreadProducerConsumer) {
  const int num_producers = 8;
  const int num_consumers = 8;
  const int items_per_producer = 10000;

  std::atomic<int> produced_count{0};
  std::atomic<int> consumed_count{0};

  std::vector<long long> produced_sum_vec(num_producers, 0);
  std::vector<long long> consumed_sum_vec(num_consumers, 0);

  std::atomic<int> thread_ready_count = 0;
  std::promise<void> go;
  auto go_fut = go.get_future().share();

  // 生产者线程：推入 items_per_producer 个元素
  auto producer = [&](int id, long long* sum) {
    thread_ready_count++;
    go_fut.get();

    for (int i = 0; i < items_per_producer; ++i) {
      int val = id * items_per_producer + i;
      *sum += val;
      this->stack_.Push(val);
      produced_count.fetch_add(1, std::memory_order_relaxed);
    }
  };

  // 消费者线程：一直 Pop，直到总消费数达到预期
  auto consumer = [&](long long* sum) {
    thread_ready_count++;
    go_fut.get();

    while (true) {
      auto p = this->stack_.Pop();
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

  while (thread_ready_count.load() != (num_producers + num_consumers));
  go.set_value();

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
TYPED_TEST(LockfreeStackTest, MixedPushPop) {
  const int threads = 12;
  const int ops_per_thread = 5000;
  std::atomic<int> net_count{0};

  std::atomic<int> thread_ready_count = 0;
  std::promise<void> go;
  auto go_fut = go.get_future().share();

  auto worker = [&](int id) {
    thread_ready_count++;
    go_fut.get();

    for (int i = 0; i < ops_per_thread; ++i) {
      if ((i + id) % 2 == 0) {
        this->stack_.Push(i);
        net_count.fetch_add(1, std::memory_order_relaxed);
      } else {
        auto p = this->stack_.Pop();
        if (p) net_count.fetch_sub(1, std::memory_order_relaxed);
      }
    }
  };

  std::vector<std::thread> ws;
  for (int i = 0; i < threads; ++i) ws.emplace_back(worker, i);

  while (thread_ready_count.load() != threads);
  go.set_value();

  for (auto& t : ws) t.join();

  int count = 0;
  while (auto p = this->stack_.Pop()) ++count;
  EXPECT_EQ(count, net_count.load());
}
