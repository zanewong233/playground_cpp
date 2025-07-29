#include <cassert>
#include <chrono>
#include <future>
#include <list>
#include <mutex>
#include <numeric>
#include <thread>
#include <vector>

#include "concurrent/thread_pool.hpp"
#include "playground/print_class/print_class.h"
#include "playground/threading/hierarchical_mutex.hpp"
#include "playground/threading/joining_thread.hpp"
#include "playground/threading/threadsafe_lookup_table.hpp"
#include "playground/threading/threadsafe_queue.hpp"
#include "playground/threading/threadsafe_stack.hpp"

using namespace playground::experiments::parallel;

class CountDownLatch {
 public:
  explicit CountDownLatch(std::ptrdiff_t count) : count_(count) {}
  void count_down() {
    std::lock_guard<std::mutex> lk(m_);
    if (--count_ == 0) cv_.notify_all();
  }
  void wait() {
    std::unique_lock<std::mutex> lk(m_);
    cv_.wait(lk, [this] { return count_ == 0; });
  }

 private:
  std::mutex m_;
  std::condition_variable cv_;
  std::ptrdiff_t count_;
};

template <typename T>
class TemplateClass {
 public:
  TemplateClass(T val) {}
  ~TemplateClass() = default;
  TemplateClass(TemplateClass&& other) : val_(std::move(other.val_)) {}
  TemplateClass& operator=(TemplateClass&& other) {
    if (this == &other) {
      return *this;
    }
    val_ = std::move(other.val_);
    return *this;
  }

  // TemplateClass(TemplateClass&) = delete;
  TemplateClass(const TemplateClass&) = delete;
  TemplateClass& operator=(TemplateClass&) = delete;

  void operator()() {}

 private:
  T val_;
};

struct MaybeThrow {
  MaybeThrow() = default;
  MaybeThrow(MaybeThrow&&) noexcept(false) {}
  MaybeThrow(const MaybeThrow&) = delete;
};

void func() {
  std::vector<FunctionWrapper> wp_vec;
  auto f1 = []() { return 10; };
  wp_vec.emplace_back(f1);
  auto f2 = [] { int a = 10; };
  wp_vec.emplace_back(f2);

  auto f3 = []() -> int {
    std::this_thread::sleep_for(std::chrono::seconds(3));
    return 10;
  };
  ThreadPool pool;
  auto fut = pool.AddTask(f3);
  int val = fut.get();

  int a = 10;
  a++;
}

void func1() {
  int a = 10;
  a++;
}

int main() {
  func();
  func1();

  int a = 10;
  a++;

  return 0;
}