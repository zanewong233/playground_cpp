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

void TestFunc() {
  std::queue<int> data_que;
  std::mutex que_mutex;
  std::condition_variable cv;

  auto worker = [&](int index) {
    while (true) {
      std::unique_lock lock(que_mutex);
      auto pred = [&data_que, &que_mutex] { return !data_que.empty(); };

      try {
        InterruptibleWait(cv, lock, pred);
      } catch (...) {
        std::cout << index << ": end task!!!!!!!" << std::endl;
        break;
      }

      int val = data_que.front();
      data_que.pop();
      std::cout << index << ": " << val << std::endl;
    }
  };
  std::vector<InterruptiableThread> threads;
  for (int i = 0; i < 4; i++) {
    threads.push_back(InterruptiableThread(std::bind(worker, i)));
  }

  for (int i = 0; i < 50; i++) {
    {
      std::unique_lock lock(que_mutex);
      data_que.push(i);
    }
    cv.notify_one();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  std::cout << "end produce data" << std::endl;
  for (int i = 0; i < threads.size(); i++) {
    threads[i].Interrupt();
  }

  std::cout << "end test func" << std::endl;

  int a = 10;
  a++;
}

void TestFunc1() {
  std::queue<int> data_que;
  std::mutex que_mutex;
  std::condition_variable_any cv;

  auto worker = [&](int index) {
    while (true) {
      std::unique_lock lock(que_mutex);
      auto pred = [&data_que, &que_mutex] { return !data_que.empty(); };

      try {
        InterruptibleWait(cv, lock);
        if (!pred()) {
          continue;
        }
      } catch (...) {
        std::cout << index << ": end task!!!!!!!" << std::endl;
        break;
      }

      int val = data_que.front();
      data_que.pop();
      std::cout << index << ": " << val << std::endl;
    }
  };
  std::vector<InterruptiableThread> threads;
  for (int i = 0; i < 4; i++) {
    threads.push_back(InterruptiableThread(std::bind(worker, i)));
  }

  for (int i = 0; i < 50; i++) {
    {
      std::unique_lock lock(que_mutex);
      data_que.push(i);
    }
    cv.notify_one();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  std::cout << "end produce data" << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(3));
  for (int i = 0; i < threads.size(); i++) {
    threads[i].Interrupt();
  }

  std::cout << "end test func" << std::endl;

  int a = 10;
  a++;
}

int main() {
  //TestFunc();
  TestFunc1();

  int a = 10;
  a++;

  return 0;
}