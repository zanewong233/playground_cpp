#include <chrono>
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
  ThreadsafeQueue<MessagePrinter> que;
  MessagePrinter tmp(233);
  que.push(tmp);
  auto res = que.empty();
  que.push(tmp);

  MessagePrinter tmp1(0);
  que.wait_and_pop(tmp1);

  int a = 10;
  a++;
}

int main() {
  func();

  int a = 10;
  a++;

  return 0;
}