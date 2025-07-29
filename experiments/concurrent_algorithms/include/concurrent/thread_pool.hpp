#ifndef PLAYGROUND_EXPERIMENTS_CONCURRENT_THREAD_POOL_H_
#define PLAYGROUND_EXPERIMENTS_CONCURRENT_THREAD_POOL_H_
#include <atomic>
#include <thread>
#include <vector>

#include "playground/threading/threadsafe_queue.hpp"
#include "threads_guard.hpp"

namespace playground::experiments::parallel {
class ThreadPool {
 public:
  ThreadPool(unsigned thread_count = -1) : done_(false), thds_guard_(threads_) {
    if (thread_count == -1) {
      thread_count = std::thread::hardware_concurrency();
      if (thread_count == 0) {
        thread_count = 2;
      }
    }

    // try 确保线程池状态一致，要么全部启动成功，要么全部停止
    try {
      threads_.reserve(thread_count);
      for (int i = 0; i < thread_count; i++) {
        threads_.emplace_back(&ThreadPool::ThreadTask, this);
      }
    } catch (...) {
      done_ = true;  // 通知已有线程尽快退出
      throw;
    }
  }
  ~ThreadPool() { done_ = true; }

  template <typename FunctionType>
  void AddTask(FunctionType&& f) {
    task_que_.push(std::function<void()>(std::forward<FunctionType>(f)));
  }

 private:
  void ThreadTask() {
    while (!done_) {
      std::function<void()> task;
      if (task_que_.try_pop(task)) {
        task();
      } else {
        std::this_thread::yield();
      }
    }
  }

 private:
  // 成员的声明顺序很重要，决定析构时是否能读取合法数据
  std::atomic_bool done_;
  playground::ThreadsafeQueue<std::function<void()>> task_que_;
  std::vector<std::thread> threads_;
  ThreadsGuard thds_guard_;
};
}  // namespace playground::experiments::parallel
#endif
