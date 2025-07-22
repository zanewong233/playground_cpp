#include <gtest/gtest.h>

#include <algorithm>
#include <forward_list>
#include <iterator>
#include <list>
#include <numeric>
#include <stdexcept>
#include <thread>
#include <vector>

#include "concurrent/find.hpp"

using namespace playground::parallel;

/* ---------- �������� ---------- */
struct ThrowCmp {
  int value;
  bool operator==(const ThrowCmp& rhs) const {
    if (rhs.value == value) throw std::runtime_error("cmp throws");
    return false;
  }
};

/* ---------- ��. ������ȷ�� ---------- */
TEST(FindParallel_Functional, EmptyRange) {
  std::vector<int> v;
  EXPECT_EQ(Find(v.begin(), v.end(), 1), v.end());
}

TEST(FindParallel_Functional, SingleElementHit) {
  std::vector<int> v{42};
  EXPECT_EQ(Find(v.begin(), v.end(), 42), v.begin());
}

TEST(FindParallel_Functional, SingleElementMiss) {
  std::vector<int> v{42};
  EXPECT_EQ(Find(v.begin(), v.end(), 0), v.end());
}

TEST(FindParallel_Functional, FrontHit) {
  std::vector<int> v{7, 8, 9};
  EXPECT_EQ(Find(v.begin(), v.end(), 7), v.begin());
}

TEST(FindParallel_Functional, BackHit) {
  std::vector<int> v{7, 8, 9};
  EXPECT_EQ(Find(v.begin(), v.end(), 9), std::prev(v.end()));
}

TEST(FindParallel_Functional, NoHit) {
  std::vector<int> v{1, 2, 3};
  EXPECT_EQ(Find(v.begin(), v.end(), 5), v.end());
}

// �ظ�ֵ��֤���ص�һ��
TEST(FindParallel_Functional, DuplicateKeepsEarliest_Vector) {
  std::vector<int> v{1, 2, 3, 2, 1};
  auto it = Find(v.begin(), v.end(), 2);
  EXPECT_EQ(it, v.begin() + 1);
}

// ͬһ�߼��� list / forward_list �ٲ�һ��
TEST(FindParallel_Functional, DuplicateKeepsEarliest_List) {
  std::list<int> lst{1, 2, 3, 2, 1};
  auto it = Find(lst.begin(), lst.end(), 2);
  EXPECT_EQ(*it, 2);
  EXPECT_EQ(std::distance(lst.begin(), it), 1);
}

TEST(FindParallel_Functional, DuplicateKeepsEarliest_ForwardList) {
  std::forward_list<int> flst{1, 2, 3, 2, 1};
  auto it = Find(flst.begin(), flst.end(), 2);
  EXPECT_EQ(*it, 2);
  EXPECT_EQ(std::distance(flst.begin(), it), 1);
}

/* ---------- ��. �������� ---------- */

// �����������У���֤�޾�̬ & �ȶ�����
TEST(FindParallel_Concurrency, LargeArrayHit) {
  constexpr std::size_t N = 1'000'000;
  std::vector<int> v(N);
  std::iota(v.begin(), v.end(), 0);
  const int target = static_cast<int>(N / 2);

  for (int i = 0; i < 100; ++i) {
    auto it = Find(v.begin(), v.end(), target);
    ASSERT_NE(it, v.end());
    EXPECT_EQ(*it, target);
  }
}

// ������������
TEST(FindParallel_Concurrency, LargeArrayMiss) {
  constexpr std::size_t N = 1'000'000;
  std::vector<int> v(N);
  std::iota(v.begin(), v.end(), 0);

  auto it = Find(v.begin(), v.end(), -1);
  EXPECT_EQ(it, v.end());
}

/* ---------- ��. �쳣���� / ��Դ ---------- */
TEST(FindParallel_Exception, PropagateCompareException) {
  std::vector<ThrowCmp> v{{1}, {2}, {3}};
  EXPECT_THROW(Find(v.begin(), v.end(), ThrowCmp{2}), std::runtime_error);
}

/* ---------- ��. ����ʱ��������ѡ���ܶ���ʾ���� ---------- */
// ���� Release �����²Ž��м����ܻع�
#ifdef NDEBUG
TEST(FindParallel_Perf, NotMuchSlowerThanStdFind) {
  constexpr std::size_t N = 2'000'000;
  std::vector<int> v(N);
  std::iota(v.begin(), v.end(), 0);

  const int target = N - 1;

  auto t0 = std::chrono::high_resolution_clock::now();
  auto it1 = std::find(v.begin(), v.end(), target);
  auto t1 = std::chrono::high_resolution_clock::now();

  auto it2 = Find(v.begin(), v.end(), target);
  auto t2 = std::chrono::high_resolution_clock::now();

  ASSERT_EQ(it1, it2);
  using ms = std::chrono::duration<double, std::milli>;
  ms std_time = t1 - t0;
  ms par_time = t2 - t1;

  // ������ʵ�� <2�� std::find ʱ�䣻�������˶�ͨ�������
  EXPECT_LT(par_time.count(), std_time.count() * 2.0);
}
#endif
