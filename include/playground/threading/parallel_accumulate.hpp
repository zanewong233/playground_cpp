#include <numeric>
#include <thread>
#include <vector>

namespace playground {
template <typename Iterator, typename T>
struct AccumulateBlock {
  void operator()(Iterator first, Iterator last, T& result) const {
    result = std::accumulate(first, last, result);
  }
};

template <typename Iterator, typename T>
T ParallelAccumulate(Iterator first, Iterator last, T init) {
  const auto length = std::distance(first, last);
  if (length == 0) return init;

  unsigned long const min_per_thread = 25;
  unsigned long const max_threads =
      (length + min_per_thread - 1) / min_per_thread;
  unsigned long const hardware_threads = std::thread::hardware_concurrency();

  unsigned long const num_threads =
      std::min(hardware_threads == 0 ? 2 : hardware_threads, max_threads);

  unsigned long const block_size = length / num_threads;

  std::vector<std::thread> threads(num_threads - 1);
  std::vector<T> results(num_threads);
  auto block_start = first;
  for (int i = 0; i < threads.size(); i++) {
    auto block_end = block_start;
    std::advance(block_end, block_size);
    threads[i] = std::thread(AccumulateBlock<Iterator, T>(), block_start,
                             block_end, std::ref(results[i]));
    block_start = block_end;
  }
  AccumulateBlock<Iterator, T>()(block_start, last, results[num_threads - 1]);

  for (int i = 0; i < threads.size(); i++) {
    threads[i].join();
  }

  return std::accumulate(results.begin(), results.end(), init);
}
}  // namespace playground
