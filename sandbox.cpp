#include <chrono>
#include <mutex>
#include <numeric>
#include <thread>
#include <vector>

#include "playground/print_class/print_class.h"
#include "playground/threading/hierarchical_mutex.hpp"
#include "playground/threading/joining_thread.hpp"
#include "playground/threading/threadsafe_stack.hpp"

using namespace playground;

HierarchicalMutex highMtx(5000), midMtx(2500), lowMtx(1000);

void highFunc() {
  std::lock_guard lock(highMtx);
  std::this_thread::sleep_for(std::chrono::seconds(10));

  int a = 10;
  a++;
}

int main() {
  JoiningThread jt(highFunc);

  std::this_thread::sleep_for(std::chrono::seconds(1));
  std::lock_guard lock(highMtx);

  int a = 10;
  a++;

  return 0;
}