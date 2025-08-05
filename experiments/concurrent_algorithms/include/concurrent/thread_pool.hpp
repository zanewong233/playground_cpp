#ifndef PLAYGROUND_EXPERIMENTS_CONCURRENT_THREAD_POOL_H_
#define PLAYGROUND_EXPERIMENTS_CONCURRENT_THREAD_POOL_H_
#include <atomic>
#include <future>
#include <memory>
#include <queue>
#include <thread>
#include <vector>

#include "playground/threading/threads_guard.hpp"
#include "playground/threading/threadsafe_queue.hpp"

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

class WorkStealingQueue {
 public:
  WorkStealingQueue() = default;
  WorkStealingQueue(const WorkStealingQueue&) = delete;
  WorkStealingQueue& operator=(const WorkStealingQueue&) = delete;

  using DataType = FunctionWrapper;
  void Push(DataType&& task) {
    std::lock_guard lock(mutex_);
    queue_.push_back(std::move(task));
  }

  bool Empty() const {
    std::lock_guard lock(mutex_);
    return queue_.empty();
  }

  bool TryPop(DataType& task) {
    std::lock_guard lock(mutex_);
    if (queue_.empty()) {
      return false;
    }

    task = std::move(queue_.front());
    queue_.pop_front();
    return true;
  }

  bool TrySteal(DataType& task) {
    std::lock_guard lock(mutex_);
    if (queue_.empty()) {
      return false;
    }

    task = std::move(queue_.back());
    queue_.pop_back();
    return true;
  }

 private:
  std::deque<DataType> queue_;
  mutable std::mutex mutex_;
};

class ThreadPool {
 public:
  ThreadPool(unsigned thread_count = -1)
      : done_(false), threads_guard_(threads_) {
    if (thread_count == -1) {
      thread_count = std::thread::hardware_concurrency();
      if (thread_count == 0) {
        thread_count = 2;
      }
    }

    // try 确保线程池状态一致，要么全部启动成功，要么全部停止
    try {
      queues_.reserve(thread_count);
      threads_.reserve(thread_count);
      for (unsigned i = 0; i < thread_count; i++) {
        queues_.push_back(std::make_unique<WorkStealingQueue>());
        threads_.emplace_back(&ThreadPool::ThreadTask, this, i);
      }
    } catch (...) {
      done_ = true;  // 通知已有线程尽快退出
      throw;
    }
  }
  ~ThreadPool() { done_ = true; }

  using TaskType = FunctionWrapper;

  template <typename F>
  std::future<std::invoke_result_t<F>> AddTask(F&& f) {
    using ResultType = std::invoke_result_t<F>;

    std::packaged_task<ResultType()> task(std::move(f));
    std::future<ResultType> fut = task.get_future();
    if (local_queue_) {
      local_queue_->Push(TaskType(std::move(task)));
    } else {
      pool_work_queue_.push(TaskType(std::move(task)));
    }
    return fut;
  }

  void RunPendingTasks() {
    TaskType task;
    if (PopTaskFromLocal(task) || PopTaskFromPool(task) ||
        PopTaskFromOtherThread(task)) {
      task();
    } else {
      std::this_thread::yield();
    }
  }

 private:
  void ThreadTask(unsigned my_index) {
    local_queue_index_ = my_index;
    local_queue_ = queues_[my_index].get();
    while (!done_) {
      RunPendingTasks();
    }
  }

  bool PopTaskFromLocal(TaskType& task) {
    return local_queue_ && local_queue_->TryPop(task);
  }

  bool PopTaskFromPool(TaskType& task) {
    return pool_work_queue_.try_pop(task);
  }

  bool PopTaskFromOtherThread(TaskType& task) {
    for (int i = 0; i < queues_.size(); i++) {
      int index = (local_queue_index_ + 1 + i) % queues_.size();
      if (queues_[index]->TrySteal(task)) {
        return true;
      }
    }
    return false;
  }

  // 成员的声明顺序很重要，决定析构时是否能读取合法数据
  std::atomic_bool done_;
  playground::ThreadsafeQueue<TaskType> pool_work_queue_;
  std::vector<std::unique_ptr<WorkStealingQueue>> queues_;
  std::vector<std::thread> threads_;
  playground::ThreadsGuard threads_guard_;

  inline static unsigned local_queue_index_ = 0;
  inline static WorkStealingQueue* local_queue_ = nullptr;
};
}  // namespace playground::experiments::parallel
#endif
