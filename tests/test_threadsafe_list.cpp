#include <gtest/gtest.h>

#include <string>
#include <thread>
#include <vector>

#include "playground/threading/threadsafe_list.hpp"

using playground::ThreadsafeList;

// Test 1: PushFront + ForEach ˳����
TEST(ThreadsafeListTest, PushAndForEach) {
  ThreadsafeList<int> list;
  list.PushFront(1);
  list.PushFront(2);
  list.PushFront(3);
  // ���� PushFront�������Դ�ͷ�������� 3,2,1
  std::vector<int> result;
  list.ForEach([&](const std::shared_ptr<int>& p) { result.push_back(*p); });
  EXPECT_EQ(result, (std::vector<int>{3, 2, 1}));
}

// Test 2: FindFirstIf ��ȷ���أ��Ҳ���
TEST(ThreadsafeListTest, FindFirstIf) {
  ThreadsafeList<std::string> list;
  list.PushFront("apple");
  list.PushFront("banana");
  // �ҵ�һ������Ϊ 5 ���ַ�����Ӧ���� "apple"
  auto p = list.FindFirstIf(
      [](const std::shared_ptr<std::string>& s) { return s->size() == 5; });
  ASSERT_TRUE(p);
  EXPECT_EQ(*p, "apple");
  // �Ҳ��� "cherry"
  auto missing = list.FindFirstIf([](const auto& s) { return *s == "cherry"; });
  EXPECT_FALSE(missing);
}

// Test 3: RemoveIf ��ν��ɾ��Ԫ��
TEST(ThreadsafeListTest, RemoveIf) {
  ThreadsafeList<int> list;
  // ���� 1..5����������Ϊ 5,4,3,2,1
  for (int i = 1; i <= 5; ++i) list.PushFront(i);
  // ɾ������ż��
  list.RemoveIf([](const std::shared_ptr<int>& p) { return (*p % 2) == 0; });
  // ������Ӧʣ�� 5,3,1
  std::vector<int> result;
  list.ForEach([&](const std::shared_ptr<int>& p) { result.push_back(*p); });
  EXPECT_EQ(result, (std::vector<int>{5, 3, 1}));
}

// Test 4: ���̲߳��� PushFront
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
  // ͳ��Ԫ�ظ�����Ӧ�������� 2*N
  int count = 0;
  list.ForEach([&](const std::shared_ptr<int>&) { ++count; });
  EXPECT_EQ(count, 2 * N);
}
