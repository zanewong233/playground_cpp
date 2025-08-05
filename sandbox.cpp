#include <algorithm>
#include <cassert>
#include <chrono>
#include <execution>
#include <future>
#include <list>
#include <mutex>
#include <numeric>
#include <thread>
#include <vector>

#include "concurrent/thread_pool.hpp"
#include "playground/print_class/print_class.h"
#include "playground/threading/hierarchical_mutex.hpp"
#include "playground/threading/interruptiable_thread.hpp"
#include "playground/threading/joining_thread.hpp"
#include "playground/threading/lockfree_stack.hpp"
#include "playground/threading/threadsafe_lookup_table.hpp"
#include "playground/threading/threadsafe_queue.hpp"
#include "playground/threading/threadsafe_stack.hpp"

using namespace playground::experiments::parallel;

void TestFunc() {
  playground::LockfreeStack<int> stack;
  stack.Push(10);
}

void TestFunc1() {}

int main() {
  TestFunc();
  TestFunc1();

  int a = 10;
  a++;

  return 0;
}