#ifndef PLAYGROUND_EXPERIMENTS_CONCURRENT_FIND_H_
#define PLAYGROUND_EXPERIMENTS_CONCURRENT_FIND_H_
#include <functional>
#include <future>
#include <iterator>
#include <thread>

#include "threads_guard.hpp"

namespace playground::parallel {
template <typename Iterator, typename MatchType>
Iterator Find(Iterator first, Iterator last, const MatchType& val) {
  struct Finder {
    void operator()(Iterator begin, Iterator end, const MatchType& val,
                    std::promise<Iterator>* result,
                    std::atomic<bool>* done_flag) {
      try {
        for (; begin != end && !done_flag->load(); ++begin) {
          if (*begin == val) {
            result->set_value(begin);
            done_flag->store(true);
            return;
          }
        }
      } catch (...) {
        try {
          result->set_exception(std::current_exception());
          done_flag->store(true);
        } catch (...) {
          // 如果promise 已经设置过，再次设置会抛异常
          // 此处直接吞掉该错误
        }
      }
    }
  };

  const auto length = static_cast<unsigned long>(std::distance(first, last));
  if (!length) {
    return last;
  }

  const unsigned long min_per_thread = 25;
  const unsigned long max_threads =
      (length + min_per_thread - 1) / min_per_thread;

  const unsigned long hardware_threads = std::thread::hardware_concurrency();
  const unsigned long num_threads =
      std::min(max_threads, std::max(2ul, hardware_threads));

  const unsigned long block_size = length / num_threads;

  std::promise<Iterator> res;
  std::atomic_bool done_flag(false);
  std::vector<std::thread> threads(num_threads - 1);
  {
    ThreadsGuard thd_guard(threads);
    Iterator block_start = first;
    for (unsigned i = 0; i < num_threads - 1; i++) {
      Iterator block_end = block_start;
      std::advance(block_end, block_size);
      threads[i] = std::thread(Finder(), block_start, block_end, std::ref(val),
                               &res, &done_flag);
      block_start = block_end;
    }
    Finder()(block_start, last, val, &res, &done_flag);
  }

  if (!done_flag.load()) {
    return last;
  }
  return res.get_future().get();
}
}  // namespace playground::parallel
#endif
