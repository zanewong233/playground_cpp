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

struct MyStruct {
  void operator()(int) {}
};

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

void func() {
  constexpr std::size_t kTasks = 300;

  ThreadPool pool{4};

  std::atomic<int> done{0};
  CountDownLatch latch(static_cast<std::ptrdiff_t>(kTasks));

  // 提交任务
  for (std::size_t i = 0; i < kTasks; ++i) {
    pool.AddTask([&] {
      // 模拟轻量工作
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      done.fetch_add(1, std::memory_order_relaxed);
      latch.count_down();
    });
  }

  // 等待所有任务完成
  latch.wait();

  // 断言
  assert(done.load(std::memory_order_acquire) == static_cast<int>(kTasks));
  std::cout << "All tasks finished. done = " << done << '\n';

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