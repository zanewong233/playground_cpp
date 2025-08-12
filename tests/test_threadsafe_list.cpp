#include <gtest/gtest.h>

#include <string>
#include <thread>
#include <vector>

#include "playground/threading/threadsafe_list.hpp"

using playground::ThreadsafeList;

// Test 1: PushFront + ForEach 顺序性
TEST(ThreadsafeListTest, PushAndForEach) {
  ThreadsafeList<int> list;
  list.PushFront(1);
  list.PushFront(2);
  list.PushFront(3);
  // 由于 PushFront，总线性从头部往后是 3,2,1
  std::vector<int> result;
  list.ForEach([&](const std::shared_ptr<int>& p) { result.push_back(*p); });
  EXPECT_EQ(result, (std::vector<int>{3, 2, 1}));
}

// Test 2: FindFirstIf 正确返回／找不到
TEST(ThreadsafeListTest, FindFirstIf) {
  ThreadsafeList<std::string> list;
  list.PushFront("apple");
  list.PushFront("banana");
  // 找第一个长度为 5 的字符串，应该是 "apple"
  auto p = list.FindFirstIf(
      [](const std::shared_ptr<std::string>& s) { return s->size() == 5; });
  ASSERT_TRUE(p);
  EXPECT_EQ(*p, "apple");
  // 找不到 "cherry"
  auto missing = list.FindFirstIf([](const auto& s) { return *s == "cherry"; });
  EXPECT_FALSE(missing);
}

// Test 3: RemoveIf 按谓词删除元素
TEST(ThreadsafeListTest, RemoveIf) {
  ThreadsafeList<int> list;
  // 插入 1..5，最终链表为 5,4,3,2,1
  for (int i = 1; i <= 5; ++i) list.PushFront(i);
  // 删除所有偶数
  list.RemoveIf([](const std::shared_ptr<int>& p) { return (*p % 2) == 0; });
  // 迭代后应剩下 5,3,1
  std::vector<int> result;
  list.ForEach([&](const std::shared_ptr<int>& p) { result.push_back(*p); });
  EXPECT_EQ(result, (std::vector<int>{5, 3, 1}));
}

// Test 4: 多线程并发 PushFront
TEST(ThreadsafeListTest, ConcurrentPush) {
  ThreadsafeList<int> list;
  constexpr int N = 1000;
  auto worker = [&](int base) {
    for (int i = 0; i < N; ++i) {
      list.PushFront(base + i);
    }
  };
  std::thread t1(worker, 0);
  std::thread t2(worker, N);
  t1.join();
  t2.join();
  // 统计元素个数，应该正好是 2*N
  int count = 0;
  list.ForEach([&](const std::shared_ptr<int>&) { ++count; });
  EXPECT_EQ(count, 2 * N);
}
