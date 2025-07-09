#include <chrono>
#include <future>
#include <list>
#include <mutex>
#include <numeric>
#include <thread>
#include <vector>

#include "playground/print_class/print_class.h"
#include "playground/threading/hierarchical_mutex.hpp"
#include "playground/threading/joining_thread.hpp"
#include "playground/threading/threadsafe_queue.hpp"
#include "playground/threading/threadsafe_stack.hpp"

using namespace playground;

void func() {
  SimpleQueue<int> que;
  auto res = que.try_pop();

  que.push(1);
  res = que.try_pop();

  que.push(2);
  que.push(3);
  que.push(4);
  que.push(5);
  res = que.try_pop();
  res = que.try_pop();
  res = que.try_pop();
  res = que.try_pop();
  res = que.try_pop();
}

int main() {
  func();

  int a = 10;
  a++;

  return 0;
}