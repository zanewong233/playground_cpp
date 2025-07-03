#include <chrono>
#include <mutex>
#include <numeric>
#include <thread>
#include <vector>

#include "playground/print_class/print_class.h"
#include "playground/threading/threadsafe_stack.hpp"

using namespace playground;

std::mutex mtx;

void foo() {
  std::lock_guard lock(mtx);
  throw std::logic_error("test");
}

void call() {
  try {
    foo();
  } catch (const std::exception&) {
  }
}

void func() {
  ThreadsafeStack<MessagePrinter> stk;
  MessagePrinter tmp;
  stk.Push(tmp);
}

int main() {
  std::thread t(call);
  std::this_thread::sleep_for(std::chrono::seconds(5));

  mtx.lock();

  int a = 10;
  a++;
  int b = 20;
  auto c = a * b;
  t.join();

  return 0;
}