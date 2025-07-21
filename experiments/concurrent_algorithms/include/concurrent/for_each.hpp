#ifndef PLAYGROUND_EXPERIMENTS_CONCURRENT_FOR_EACH_H_
#define PLAYGROUND_EXPERIMENTS_CONCURRENT_FOR_EACH_H_
#include <future>
#include <iterator>
#include <thread>
#include <vector>

namespace playground::parallel {
class ThreadingGuard {
 public:
  ThreadingGuard(std::vector<thread>& ths) : threads_(ths) {}
  ~ThreadingGuard() {
    for (auto it : threads_) {
      it.join();
    }
  }

  ThreadingGuard(const ThreadingGuard&) = delete;
  ThreadingGuard& operator=(const ThreadingGuard&) = delete;

 private:
  std::vector<std::thread>& threads_;
};

template <typename Iterator, typename Func>
void ParallelForEach(Iterator first, Iterator last, Func f) {
  unsigned length = std::distance(first, last);
  if (!length) return;

  // 对数据进行分块
  constexpr unsigned min_pre_thread = 25;
  unsigned block_count = (length + min_pre_thread - 1) / min_pre_thread;

  unsigned num_thread = std::thread::hardware_concurrency();
  num_thread = std::min(block_count, std::max(2, num_thread));
  unsigned block_size = length / block_count;

  // 按照块大小异步执行
  std::vector<std::future<void>> fut_list;
  std::vector<std::thread> threads;
  Iterator it_start = first;
  for (unsigned i = 0; i < num_thread - 1; i++) {
    Iterator it_end = std::advance(it_start, block_size);
    std::packaged_task<void()> task = [it_start, it_end, f] {
      for (auto it = it_start; it != it_end; ++it) {
        f(*it);
      }
    };

    fut_list.push_back(task.get_future());
    threads.emplace_back(task);
    it_start = it_end;
  }
  ThreadingGuard thd_guard(threads);
  for (auto it = it_start; it != last; ++it) {
    f(*it);
  }

  // 阻塞获取结果
  for (auto fut : fut_list) {
    fut.get();
  }
}
}  // namespace playground::parallel
#endif
