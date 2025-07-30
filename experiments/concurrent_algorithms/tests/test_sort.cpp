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

// 1. ���б�
TEST(SorterThreadPoolTest, EmptyList) {
  SorterThreadPool<int> pool;
  std::list<int> input;
  auto output = pool.DoSort(input);
  EXPECT_TRUE(output.empty());
}

// 2. ��Ԫ���б�
TEST(SorterThreadPoolTest, SingleElement) {
  SorterThreadPool<int> pool;
  std::list<int> input = {42};
  auto output = pool.DoSort(input);
  ASSERT_EQ(output.size(), 1u);
  EXPECT_EQ(output.front(), 42);
}

// 3. ���ź�����б�
TEST(SorterThreadPoolTest, AlreadySorted) {
  SorterThreadPool<int> pool;
  std::list<int> input = {1, 2, 3, 4, 5};
  auto output = pool.DoSort(input);
  EXPECT_EQ(output, input);
}

// 4. �����б�
TEST(SorterThreadPoolTest, ReverseOrder) {
  SorterThreadPool<int> pool;
  std::list<int> input = {5, 4, 3, 2, 1};
  std::list<int> expect = {1, 2, 3, 4, 5};
  auto output = pool.DoSort(input);
  EXPECT_EQ(output, expect);
}

// 5. ���ظ�Ԫ�ص������б�
TEST(SorterThreadPoolTest, WithDuplicates) {
  SorterThreadPool<int> pool;
  std::list<int> input = {3, 1, 2, 3, 2, 1};
  std::list<int> expect = {1, 1, 2, 2, 3, 3};
  auto output = pool.DoSort(input);
  EXPECT_EQ(output, expect);
}

// 6. �Զ�������
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

// 7. ʹ���Զ���Ƚ������������� DoSort ֧�ִ� comparator��
// ��� DoSort ֧�ֵڶ���ģ����������أ��ɲ��Խ���
#if 0
TEST(SorterThreadPoolTest, CustomComparator) {
    SorterThreadPool<int> pool;
    std::list<int> input = {1, 3, 2, 5, 4};
    auto output = pool.DoSort(input, std::greater<int>());
    std::list<int> expect = {5,4,3,2,1};
    EXPECT_EQ(output, expect);
}
#endif

// 8. ���ģ����ѹ�����ԣ��������ܲ��ԣ�����ʱ��
TEST(SorterThreadPoolTest, LargeRandom) {
  SorterThreadPool<int> pool;
  std::list<int> input;
  const int N = 10000;
  for (int i = N; i >= 1; --i) input.push_back(i);
  auto output = pool.DoSort(input);
  int prev = 0;
  for (int x : output) {
    EXPECT_GT(x, prev);
    prev = x;
  }
  EXPECT_EQ(prev, N);
}

// 9. ������ȫ�Բ��ԣ����߳�ͬʱ���ã���ͬʵ����
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
