#ifndef PLAYGROUND_EXPERIMENTS_CONCURRENT_THREAD_POOL_H_
#define PLAYGROUND_EXPERIMENTS_CONCURRENT_THREAD_POOL_H_
#include <atomic>
#include <future>
#include <memory>
#include <thread>
#include <vector>

#include "playground/threading/threadsafe_queue.hpp"
#include "threads_guard.hpp"

namespace playground::experiments::parallel {
class FunctionWrapper {
 public:
  template <typename F>
  explicit FunctionWrapper(F&& f) : impl_(new ImpType(std::move(f))) {}
  FunctionWrapper() = default;
  FunctionWrapper(FunctionWrapper&& other) : impl_(std::move(other.impl_)) {}
  FunctionWrapper& operator=(FunctionWrapper&& other) {
    if (&other == this) {
      return *this;
    }
    impl_ = std::move(other.impl_);
    return *this;
  }
  FunctionWrapper(const FunctionWrapper&) = delete;
  FunctionWrapper(FunctionWrapper&) = delete;
  FunctionWrapper& operator=(const FunctionWrapper&) = delete;

  void operator()() { impl_->call(); }

 private:
  struct ImpBase {
    virtual void call() = 0;
    virtual ~ImpBase() = default;
  };
  template <typename F>
  struct ImpType : ImpBase {
    ImpType(F&& f) : f_(std::move(f)) {}
    void call() override { f_(); }
    F f_;
  };

  std::unique_ptr<ImpBase> impl_;
};

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
      for (unsigned i = 0; i < thread_count; i++) {
        threads_.emplace_back(&ThreadPool::ThreadTask, this);
      }
    } catch (...) {
      done_ = true;  // 通知已有线程尽快退出
      throw;
    }
  }
  ~ThreadPool() { done_ = true; }

  template <typename F>
  std::future<std::invoke_result_t<F>> AddTask(F&& f) {
    std::packaged_task task(std::move(f));
    auto fut = task.get_future();
    task_que_.push(FunctionWrapper(std::move(task)));
    return fut;
  }

 private:
  void ThreadTask() {
    while (!done_) {
      FunctionWrapper task;
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
  playground::ThreadsafeQueue<FunctionWrapper> task_que_;
  std::vector<std::thread> threads_;
  ThreadsGuard thds_guard_;
};
}  // namespace playground::experiments::parallel
#endif
