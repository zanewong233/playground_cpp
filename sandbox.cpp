#include <cassert>
#include <chrono>
#include <future>
#include <list>
#include <mutex>
#include <numeric>
#include <thread>
#include <vector>

#include "concurrent/interruptiable_thread.hpp"
#include "concurrent/thread_pool.hpp"
#include "playground/print_class/print_class.h"
#include "playground/threading/hierarchical_mutex.hpp"
#include "playground/threading/joining_thread.hpp"
#include "playground/threading/threadsafe_lookup_table.hpp"
#include "playground/threading/threadsafe_queue.hpp"
#include "playground/threading/threadsafe_stack.hpp"

using namespace playground::experiments::parallel;

void func() {
  std::promise<std::function<void()>> interrupt_point_func_promise;
  auto future = interrupt_point_func_promise.get_future().share();
  auto worker = [&future]() {
    while (true) {
      // 模拟任务执行
      std::this_thread::sleep_for(std::chrono::seconds(2));
      std::cout << "execute task" << std::endl;
      if (auto interrupt_point = future.get()) {
        try {
          interrupt_point();
        } catch (...) {
          std::cout << "end task!" << std::endl;
          break;
        }
      }
    }
  };
  InterruptiableThread thread_(worker);
  std::function<void()> interrupt_func =
      std::bind(&InterruptiableThread::InterruptPoint, &thread_);
  interrupt_point_func_promise.set_value(interrupt_func);

  std::this_thread::sleep_for(std::chrono::seconds(6));
  thread_.Interrupt();
  std::cout << "interrupted task!" << std::endl;
  thread_.Join();

  std::cout << "end test func" << std::endl;

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