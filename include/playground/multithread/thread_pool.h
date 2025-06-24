#pragma once
#include <condition_variable>
#include <functional>
#include <future>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
 public:
  ThreadPool(unsigned int num = -1);
  ~ThreadPool() noexcept;

  using Task = std::function<void()>;

  template <typename F, typename... Args>
  auto addTask(F&& fn, Args&&... args)
      -> std::future<std::invoke_result_t<F, Args...>>;

  void stop();
  void abort();

 private:
  std::vector<std::thread> threads_;
  std::queue<Task> task_list_;
  std::condition_variable cv_;
  std::mutex mtx_;
  bool stop_ = false;
};

template <typename F, typename... Args>
auto ThreadPool::addTask(F&& fn, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>> {
  using R = std::invoke_result_t<F, Args...>;
  auto task = std::make_shared<std::packaged_task<R()>>(
      std::bind(std::forward<F>(fn), std::forward<Args>(args)...));
  std::future<R> res = task->get_future();
  {
    std::lock_guard<std::mutex> lock(mtx_);
    if (stop_) throw std::logic_error("Thread pool has stopped!");
    auto lambda = [task] { (*task)(); };
    task_list_.push(lambda);
    cv_.notify_one();
  }
  return res;
}

void test_thread_pool();
