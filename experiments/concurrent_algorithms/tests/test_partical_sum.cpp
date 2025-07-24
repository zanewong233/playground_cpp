#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <forward_list>
#include <iterator>
#include <list>
#include <numeric>
#include <random>
#include <thread>
#include <vector>

#include "concurrent/partial_sum.hpp"

using namespace playground::parallel;

//#include <crtdbg.h>

//int main(int argc, char** argv) {
//  // 打开 CRT 堆检查
//  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF |
//                 _CRTDBG_LEAK_CHECK_DF);
//  ::testing::InitGoogleTest(&argc, argv);
//  ::testing::GTEST_FLAG(filter) =
//      "PartialSumParallel_CorrectnessVsStd.LargeVectorOrdered";
//  return RUN_ALL_TESTS();
//}

/* ---------- Ⅰ. 功能正确性 ---------- */
TEST(PartialSumParallel_Functional, EmptyRange) {
  std::vector<int> v;
  PartialSum(v.begin(), v.end());
  EXPECT_TRUE(v.empty());
}

TEST(PartialSumParallel_Functional, SingleElement) {
  std::vector<int> v{42};
  PartialSum(v.begin(), v.end());
  EXPECT_EQ(v[0], 42);
}

TEST(PartialSumParallel_Functional, SmallVectorInt) {
  std::vector<int> v{1, 2, 3, 4};
  PartialSum(v.begin(), v.end());
  std::vector<int> expected{1, 3, 6, 10};
  EXPECT_EQ(v, expected);
}

TEST(PartialSumParallel_Functional, SmallVectorDouble) {
  std::vector<double> v{0.1, 0.2, 0.3};
  PartialSum(v.begin(), v.end());
  std::vector<double> expected{0.1, 0.3, 0.6};
  EXPECT_TRUE(
      std::equal(v.begin(), v.end(), expected.begin(),
                 [](double a, double b) { return std::abs(a - b) < 1e-9; }));
}

TEST(PartialSumParallel_Functional, ListIteratorCategory) {
  std::list<int> lst{2, 2, 2, 2};
  PartialSum(lst.begin(), lst.end());
  std::vector<int> result(lst.begin(), lst.end());
  std::vector<int> expected{2, 4, 6, 8};
  EXPECT_EQ(result, expected);
}

TEST(PartialSumParallel_Functional, ForwardListIteratorCategory) {
  std::forward_list<int> flst{3, 3, 3};
  PartialSum(flst.begin(), flst.end());
  std::vector<int> result(flst.begin(), flst.end());
  std::vector<int> expected{3, 6, 9};
  EXPECT_EQ(result, expected);
}

/* ---------- Ⅱ. 与 std::partial_sum 一致的大规模数组 ---------- */

TEST(PartialSumParallel_CorrectnessVsStd, LargeVectorOrdered) {
   constexpr std::size_t N = 1'000'000;
  std::vector<int> data(N);
  std::iota(data.begin(), data.end(), 0);

  std::vector<int> expect = data;

  std::partial_sum(expect.begin(), expect.end(), expect.begin());
  PartialSum(data.begin(), data.end());

  EXPECT_EQ(data, expect);
}

TEST(PartialSumParallel_CorrectnessVsStd, LargeVectorRandom) {
   constexpr std::size_t N = 1'000'000;
  std::vector<int> data(N);
  std::mt19937 rng(123);
  std::uniform_int_distribution<int> dist(-50, 50);
  std::generate(data.begin(), data.end(), [&] { return dist(rng); });

  std::vector<int> expect = data;

  PartialSum(data.begin(), data.end());
  std::partial_sum(expect.begin(), expect.end(), expect.begin());

  EXPECT_EQ(data, expect);
}

/* ---------- Ⅲ. 并发可重入：多线程同时调用 ---------- */

TEST(PartialSumParallel_Reentrancy, ConcurrentInvocations) {
  constexpr std::size_t N = 200'000;
  std::vector<int> a(N), b(N);
  std::iota(a.begin(), a.end(), 1);
  std::iota(b.begin(), b.end(), 1);

  std::thread t1(
      [](std::vector<int>& vec) { PartialSum(vec.begin(), vec.end()); },
      std::ref(a));
  std::thread t2(
      [](std::vector<int>& vec) { PartialSum(vec.begin(), vec.end()); },
      std::ref(b));
  t1.join();
  t2.join();

  // 结果应分别等于标准库输出
  std::vector<int> expected(N);
  std::iota(expected.begin(), expected.end(), 1);
  std::partial_sum(expected.begin(), expected.end(), expected.begin());

  EXPECT_EQ(a, expected);
  EXPECT_EQ(b, expected);
}

/* ---------- Ⅳ. 简单性能回归（Release 时） ---------- */

#ifdef NDEBUG
TEST(PartialSumParallel_Perf, NotMuchSlowerThanStd) {
  constexpr std::size_t N = 2'000'000;
  std::vector<int> v(N);
  std::iota(v.begin(), v.end(), 1);

  std::vector<int> v_copy = v;  // 给 std::partial_sum 用

  auto t0 = std::chrono::high_resolution_clock::now();
  PartialSum(v.begin(), v.end());
  auto t1 = std::chrono::high_resolution_clock::now();
  std::partial_sum(v_copy.begin(), v_copy.end(), v_copy.begin());
  auto t2 = std::chrono::high_resolution_clock::now();

  using ms = std::chrono::duration<double, std::milli>;
  ms par_time = t1 - t0;
  ms std_time = t2 - t1;

  // 并行算法不应比顺序版慢 2 倍以上
  EXPECT_LT(par_time.count(), std_time.count() * 2.0);
  EXPECT_EQ(v, v_copy);  // 结果仍要一致
}
#endif
