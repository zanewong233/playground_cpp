// 文件：lockfree_stacks_typed_test.cpp
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <numeric>
#include <random>
#include <thread>
#include <unordered_map>
#include <vector>

#include "playground/threading/lockfree_stack_hazard_pointer.hpp"
#include "playground/threading/lockfree_stack_reference_counting.hpp"
#include "playground/threading/lockfree_stack_shared_ptr.hpp"
#include "playground/threading/lockfree_statck_delete_list.hpp"

using playground::LockfreeStackDeleteList;
using playground::LockfreeStackHazardPointer;
using playground::LockfreeStackReferenceCounting;
using playground::LockfreeStackSharedPtr;

// ========== 统一 Pop 接口的适配层（C++17 检测惯用法） ==========

namespace test_helpers {

template <class S, class V>
struct has_bool_pop_out {
 private:
  template <class X>
  static auto test(int)
      -> decltype(std::declval<X&>().Pop(std::declval<V&>()), std::true_type{});
  template <class>
  static auto test(...) -> std::false_type;

 public:
  static constexpr bool value = decltype(test<S>(0))::value;
};

// push 直接用同名函数
template <class S, class V>
inline void push(S& s, const V& v) {
  s.Push(v);
}

// 统一的 pop：将弹出的值写入 out，并返回是否成功
template <class S, class V>
inline bool pop(S& s, V& out) {
  if constexpr (has_bool_pop_out<S, V>::value) {
    return s.Pop(out);
  } else {
    auto p = s.Pop();
    if (p) {
      out = *p;
      return true;
    }
    return false;
  }
}

}  // namespace test_helpers

// ========== 辅助：起跑线屏障（C++17） ==========
struct StartGate {
  std::mutex m;
  std::condition_variable cv;
  bool go{false};
  void wait() {
    std::unique_lock<std::mutex> lk(m);
    cv.wait(lk, [&] { return go; });
  }
  void open() {
    std::lock_guard<std::mutex> lk(m);
    go = true;
    cv.notify_all();
  }
};

// ========== 你的 typed suite（元素类型为 int） ==========
using Implementations = ::testing::Types<
    LockfreeStackDeleteList<int>, LockfreeStackHazardPointer<int>,
    LockfreeStackSharedPtr<int>, LockfreeStackReferenceCounting<int>>;

template <typename StackT>
class LockfreeStackTest : public ::testing::Test {
 protected:
  StackT stack_;
};
TYPED_TEST_SUITE(LockfreeStackTest, Implementations);

// ---- 基础功能 & 边界 ----
TYPED_TEST(LockfreeStackTest, BasicPushPopLifo) {
  using S = TypeParam;
  S& s = this->stack_;
  for (int i = 0; i < 5; ++i) test_helpers::push(s, i);  // 0..4
  for (int expect = 4; expect >= 0; --expect) {
    int v = -1;
    ASSERT_TRUE(test_helpers::pop(s, v));
    EXPECT_EQ(v, expect);
  }
  int v = -1;
  EXPECT_FALSE(test_helpers::pop(s, v));  // 空
}

TYPED_TEST(LockfreeStackTest, PopFromEmpty) {
  using S = TypeParam;
  S& s = this->stack_;
  int v = 0;
  EXPECT_FALSE(test_helpers::pop(s, v));
}

// ---- MPMC：无丢失/无重复 ----
TYPED_TEST(LockfreeStackTest, MPMCIntegrityNoLossNoDup) {
  using S = TypeParam;
  S& s = this->stack_;
  const int P = 4, C = 4, PER_P = 10000, N = P * PER_P;

  StartGate gate;
  std::atomic<int> produced{0}, consumed{0};
  std::vector<std::thread> producers, consumers;

  std::mutex mu;
  std::unordered_map<int, int> seen;

  for (int pi = 0; pi < P; ++pi) {
    producers.emplace_back([&, pi] {
      gate.wait();
      const int base = pi * PER_P;
      for (int k = 0; k < PER_P; ++k) {
        test_helpers::push(s, base + k);
        produced.fetch_add(1, std::memory_order_relaxed);
      }
    });
  }

  for (int ci = 0; ci < C; ++ci) {
    consumers.emplace_back([&] {
      gate.wait();
      for (;;) {
        int v = -1;
        if (test_helpers::pop(s, v)) {
          std::lock_guard<std::mutex> lk(mu);
          seen[v]++;
          consumed.fetch_add(1, std::memory_order_relaxed);
        } else {
          if (produced.load(std::memory_order_relaxed) < N) continue;
          int u = -1;
          if (!test_helpers::pop(s, u)) break;
          std::lock_guard<std::mutex> lk(mu);
          seen[u]++;
          consumed.fetch_add(1, std::memory_order_relaxed);
        }
      }
    });
  }

  gate.open();
  for (auto& t : producers) t.join();
  for (auto& t : consumers) t.join();

  ASSERT_EQ(consumed.load(), N);
  ASSERT_EQ(static_cast<int>(seen.size()), N);
  for (int i = 0; i < N; ++i) {
    auto it = seen.find(i);
    ASSERT_TRUE(it != seen.end());
    EXPECT_EQ(it->second, 1) << "value " << i << " duplicated!";
  }
}

// ---- 强压力：Pop 后立即 Push（高 ABA 压力） ----
TYPED_TEST(LockfreeStackTest, StressABA_NoCrash_NoLoss) {
  using S = TypeParam;
  S& s = this->stack_;
  constexpr int N = 20000;
  constexpr int T = 8;
  constexpr int ROUNDS = 20000;

  for (int i = 0; i < N; ++i) test_helpers::push(s, i);

  StartGate gate;
  std::vector<std::thread> ts;
  for (int ti = 0; ti < T; ++ti) {
    ts.emplace_back([&] {
      gate.wait();
      for (int r = 0; r < ROUNDS; ++r) {
        int v;
        if (test_helpers::pop(s, v)) {
          test_helpers::push(s, v);
          int w;
          if (test_helpers::pop(s, w)) {
            test_helpers::push(s, w);
          }
        }
        if ((r & 1023) == 0) std::this_thread::yield();
      }
    });
  }

  gate.open();
  for (auto& th : ts) th.join();

  // 清空并检查 0..N-1 各出现一次
  std::vector<char> mark(N, 0);
  int cnt = 0, x;
  while (test_helpers::pop(s, x)) {
    ASSERT_GE(x, 0);
    ASSERT_LT(x, N);
    ASSERT_EQ(mark[x], 0) << "duplicate value " << x;
    mark[x] = 1;
    ++cnt;
  }
  ASSERT_EQ(cnt, N);
  for (int i = 0; i < N; ++i) {
    ASSERT_EQ(mark[i], 1) << "missing value " << i;
  }
}

// ========== 针对资源释放（Counted）建立第二个 typed suite ==========

struct Counted {
  static std::atomic<int> live;
  int v;
  explicit Counted(int x = 0) : v(x) {
    live.fetch_add(1, std::memory_order_relaxed);
  }
  Counted(const Counted& o) : v(o.v) {
    live.fetch_add(1, std::memory_order_relaxed);
  }
  Counted& operator=(const Counted& o) {
    v = o.v;
    return *this;
  }
  ~Counted() { live.fetch_sub(1, std::memory_order_relaxed); }
};
std::atomic<int> Counted::live{0};

using ImplementationsCounted = ::testing::Types<
    LockfreeStackDeleteList<Counted>, LockfreeStackHazardPointer<Counted>,
    LockfreeStackSharedPtr<Counted>, LockfreeStackReferenceCounting<Counted>>;

template <typename StackT>
class LockfreeStackCountedTest : public ::testing::Test {
 protected:
  StackT stack_;
};
TYPED_TEST_SUITE(LockfreeStackCountedTest, ImplementationsCounted);

TYPED_TEST(LockfreeStackCountedTest, Reclaim_AllReleasedEventually) {
  using S = TypeParam;
  {
    S s;
    const int TNUM = 6, PER_T = 5000;
    StartGate gate;
    std::vector<std::thread> ths;
    for (int t = 0; t < TNUM; ++t) {
      ths.emplace_back([&, t] {
        gate.wait();
        for (int i = 0; i < PER_T; ++i) {
          test_helpers::push(s, Counted(i + t * PER_T));
          Counted tmp;
          if (test_helpers::pop(s, tmp) && (i & 1)) {
            test_helpers::push(s, tmp);
          }
        }
      });
    }
    gate.open();
    for (auto& th : ths) th.join();
    Counted tmp;
    while (test_helpers::pop(s, tmp)) {
    }  // 清空
  }
  EXPECT_EQ(Counted::live.load(std::memory_order_relaxed), 0);
}

// ========== 针对可见性（Big）建立第三个 typed suite ==========

struct Big {
  int a;
  int b;
  int c;
};
using ImplementationsBig = ::testing::Types<
    LockfreeStackDeleteList<Big>, LockfreeStackHazardPointer<Big>,
    LockfreeStackSharedPtr<Big>, LockfreeStackReferenceCounting<Big>>;

template <typename StackT>
class LockfreeStackBigTest : public ::testing::Test {
 protected:
  StackT stack_;
};
TYPED_TEST_SUITE(LockfreeStackBigTest, ImplementationsBig);

TYPED_TEST(LockfreeStackBigTest, VisibilityAfterPush) {
  using S = TypeParam;
  S s;
  constexpr int N = 10000;

  std::atomic<bool> start{false};
  std::atomic<int> ok{0};

  std::thread prod([&] {
    start.store(true, std::memory_order_relaxed);
    for (int i = 0; i < N; ++i) {
      Big x{i, i + 1, i + 2};
      test_helpers::push(s, x);
    }
  });

  std::thread cons([&] {
    while (!start.load(std::memory_order_relaxed)) {
    }
    int got = 0;
    for (;;) {
      Big v{};
      if (!test_helpers::pop(s, v)) {
        if (got < N) continue;
        break;
      }
      if (v.b == v.a + 1 && v.c == v.a + 2) ++ok;
      ++got;
      if (got == N) break;
    }
  });

  prod.join();
  cons.join();
  EXPECT_EQ(ok.load(), N);
}
