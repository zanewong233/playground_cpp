#include "playground/threading/thread_pool.h"

#include <iostream>

namespace playground {
ThreadPool::ThreadPool(unsigned int num) {
  unsigned int hardware_threads = std::thread::hardware_concurrency();

  if (num == static_cast<unsigned int>(-1)) {
    num = hardware_threads;
  } else if (hardware_threads != 0) {
    const unsigned int maxNum = hardware_threads * 2;
    if (num > maxNum) {
      num = maxNum;
    }
  }
  if (num == 0) {  // 确保至少有一个线程
    num = 1;
  }

  threads_.reserve(num);
  for (unsigned int i = 0; i < num; i++) {
    threads_.emplace_back([this]() {
      while (true) {
        Task task;
        {
          std::unique_lock<std::mutex> lock(mtx_);
          cv_.wait(lock, [this]() { return !task_list_.empty() || stop_; });
          if (task_list_.empty() && stop_) break;
          task = std::move(task_list_.front());
          task_list_.pop();
        }

        try {
          task();
        } catch (std::exception& e) {
          std::cerr << "ThreadPool task exception:" << e.what() << std::endl;
        }
      }
    });
  }
}

ThreadPool::~ThreadPool() noexcept {
  stop();
  for (int i = 0; i < threads_.size(); i++) {
    threads_[i].join();
  }
}

void ThreadPool::stop() {
  {
    std::lock_guard<std::mutex> lock(mtx_);
    stop_ = true;
  }
  cv_.notify_all();
}

void ThreadPool::abort() {
  {
    std::lock_guard<std::mutex> lock(mtx_);
    std::queue<Task> empty_que;
    empty_que.swap(task_list_);
    stop_ = true;
  }
  cv_.notify_all();
}

// =============================test=============================
namespace {
void sum(const std::vector<int>& vec, int* res) {
  *res = 0;
  for (int i = 0; i < vec.size(); i++) {
    *res += vec[i];
  }
}

int sum1(const std::vector<int>& vec) {
  int res = 0;
  for (int i = 0; i < vec.size(); i++) {
    res += vec[i];
  }
  return res;
}
}  // namespace

void test_thread_pool() {
  {
    std::vector<int> vec1(100, 314);
    std::vector<int> vec2(100, 356);
    ThreadPool pool;
    auto res1 = pool.addTask(sum1, vec1);
    auto res2 = pool.addTask(sum1, vec2);

    auto val1 = res1.get();
    auto val2 = res2.get();

    int a = 10;
    a++;
  }

  {
    std::vector<int> vec1(100, 314);
    std::vector<int> vec2(100, 356);
    int res1 = 0, res2 = 0;

    ThreadPool::Task tk1 = std::bind(sum, vec1, &res1);
    ThreadPool::Task tk2 = std::bind(sum, vec2, &res2);

    {
      ThreadPool pool;
      pool.addTask(tk1);
      pool.addTask(tk2);
    }
  }

  int a = 10;
  a++;
}
}  // namespace playground
