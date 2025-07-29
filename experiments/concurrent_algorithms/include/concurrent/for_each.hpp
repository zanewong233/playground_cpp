#ifndef PLAYGROUND_EXPERIMENTS_CONCURRENT_FOR_EACH_H_
#define PLAYGROUND_EXPERIMENTS_CONCURRENT_FOR_EACH_H_
#include <future>
#include <iterator>
#include <thread>
#include <vector>

#include "threads_guard.hpp"

namespace playground::experiments::parallel {
template <typename Iterator, typename Func>
void ForEach(Iterator first, Iterator last, Func f) {
  auto length = static_cast<unsigned long>(std::distance(first, last));
  if (!length) {
    return;
  }

  // 对数据进行分块
  constexpr unsigned long min_pre_thread = 25;
  const unsigned long max_threads =
      (length + min_pre_thread - 1) / min_pre_thread;

  const unsigned long hardware_threads = std::thread::hardware_concurrency();

  const unsigned long num_threads =
      std::min(max_threads, std::max(2ul, hardware_threads));

  const unsigned long block_size = length / num_threads;

  // 按照块大小异步执行
  // 先申请空间可以防止后续push_back抛出异常
  std::vector<std::future<void>> fut_list(num_threads - 1);
  std::vector<std::thread> threads(num_threads - 1);
  ThreadsGuard thd_guard(threads);

  Iterator block_start = first;
  for (unsigned long i = 0; i < num_threads - 1; i++) {
    Iterator block_end = block_start;
    std::advance(block_end, block_size);
    std::packaged_task<void()> task([block_start, block_end, f]() {
      std::for_each(block_start, block_end, f);
    });

    fut_list[i] = task.get_future();
    threads[i] = std::thread(std::move(task));
    block_start = block_end;
  }
  std::for_each(block_start, last, f);

  // 阻塞等待完成
  for (auto& fut : fut_list) {
    fut.get();
  }
}

template <typename Iterator, typename Func>
void ForEachRecursive(Iterator first, Iterator last, Func f) {
  const auto length = std::distance(first, last);
  if (!length) {
    return;
  }

  const auto min_per_thread = 25ul;
  if (length < (2 * min_per_thread)) {
    std::for_each(first, last, f);
  } else {
    const auto mid_point = first + length / 2;
    std::future<void> first_half =
        std::async(ForEachRecursive, first, mid_point, f);

    ForEachRecursive(mid_point, last, f);
    first_half.get();
  }
}

}  // namespace playground::experiments::parallel
#endif
