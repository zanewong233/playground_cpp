#include <gtest/gtest.h>

#include <algorithm>
#include <forward_list>
#include <iterator>
#include <list>
#include <numeric>
#include <stdexcept>
#include <thread>
#include <vector>

#include "concurrent/sort.hpp"

using namespace playground::experiments::parallel;

// 1. 空列表
TEST(SorterThreadPoolTest, EmptyList) {
  SorterThreadPool<int> pool;
  std::list<int> input;
  auto output = pool.DoSort(input);
  EXPECT_TRUE(output.empty());
}

// 2. 单元素列表
TEST(SorterThreadPoolTest, SingleElement) {
  SorterThreadPool<int> pool;
  std::list<int> input = {42};
  auto output = pool.DoSort(input);
  ASSERT_EQ(output.size(), 1u);
  EXPECT_EQ(output.front(), 42);
}

// 3. 已排好序的列表
TEST(SorterThreadPoolTest, AlreadySorted) {
  SorterThreadPool<int> pool;
  std::list<int> input = {1, 2, 3, 4, 5};
  auto output = pool.DoSort(input);
  EXPECT_EQ(output, input);
}

// 4. 逆序列表
TEST(SorterThreadPoolTest, ReverseOrder) {
  SorterThreadPool<int> pool;
  std::list<int> input = {5, 4, 3, 2, 1};
  std::list<int> expect = {1, 2, 3, 4, 5};
  auto output = pool.DoSort(input);
  EXPECT_EQ(output, expect);
}

// 5. 含重复元素的乱序列表
TEST(SorterThreadPoolTest, WithDuplicates) {
  SorterThreadPool<int> pool;
  std::list<int> input = {3, 1, 2, 3, 2, 1};
  std::list<int> expect = {1, 1, 2, 2, 3, 3};
  auto output = pool.DoSort(input);
  EXPECT_EQ(output, expect);
}

// 6. 自定义类型
struct Person {
  std::string name;
  int age;
  bool operator<(Person const& o) const { return age < o.age; }
  bool operator>(Person const& o) const { return age > o.age; }
  bool operator==(Person const& o) const {
    return age == o.age && name == o.name;
  }
};

TEST(SorterThreadPoolTest, CustomType) {
  SorterThreadPool<Person> pool;
  std::list<Person> input = {{"Alice", 30}, {"Bob", 25}, {"Carol", 40}};
  auto output = pool.DoSort(input);
  std::vector<int> ages;
  for (auto const& p : output) ages.push_back(p.age);
  std::vector<int> expect = {25, 30, 40};
  EXPECT_EQ(ages, expect);
}

// 7. 使用自定义比较器（假设重载 DoSort 支持传 comparator）
// 如果 DoSort 支持第二个模板参数或重载，可测试降序
#if 0
TEST(SorterThreadPoolTest, CustomComparator) {
    SorterThreadPool<int> pool;
    std::list<int> input = {1, 3, 2, 5, 4};
    auto output = pool.DoSort(input, std::greater<int>());
    std::list<int> expect = {5,4,3,2,1};
    EXPECT_EQ(output, expect);
}
#endif

// 8. 大规模性能压力测试（仅做功能测试，不计时）
TEST(SorterThreadPoolTest, LargeRandom) {
  SorterThreadPool<int> pool;
  std::list<int> input;
  const int N = 10'000;
  for (int i = N; i >= 1; --i) input.push_back(i);
  auto output = pool.DoSort(input);
  int prev = 0;
  for (int x : output) {
    EXPECT_GT(x, prev);
    prev = x;
  }
  EXPECT_EQ(prev, N);
}

// 9. 并发安全性测试：多线程同时调用（不同实例）
TEST(SorterThreadPoolTest, ConcurrentCalls) {
  auto worker = []() {
    SorterThreadPool<int> pool;
    std::list<int> input = {3, 1, 2};
    auto o = pool.DoSort(input);
    EXPECT_EQ(o, (std::list<int>{1, 2, 3}));
  };
  std::thread t1(worker), t2(worker), t3(worker);
  t1.join();
  t2.join();
  t3.join();
}
