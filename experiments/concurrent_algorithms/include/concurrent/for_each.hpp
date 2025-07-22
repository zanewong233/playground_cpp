#ifndef PLAYGROUND_EXPERIMENTS_CONCURRENT_FOR_EACH_H_
#define PLAYGROUND_EXPERIMENTS_CONCURRENT_FOR_EACH_H_
#include <future>
#include <iterator>
#include <thread>
#include <vector>

namespace playground::parallel {
class ThreadingGuard {
 public:
  ThreadingGuard(std::vector<std::thread>& ths) : threads_(ths) {}
  ~ThreadingGuard() {
    for (auto& it : threads_) {
      if (it.joinable()) {
        it.join();
      }
    }
  }

  ThreadingGuard(const ThreadingGuard&) = delete;
  ThreadingGuard& operator=(const ThreadingGuard&) = delete;

 private:
  std::vector<std::thread>& threads_;
};

template <typename Iterator, typename Func>
void ParallelForEach(Iterator first, Iterator last, Func f) {
  auto length = static_cast<unsigned long>(std::distance(first, last));
  if (!length) {
    return;
  }

  // �����ݽ��зֿ�
  constexpr unsigned long min_pre_thread = 25;
  const unsigned long max_threads =
      (length + min_pre_thread - 1) / min_pre_thread;

  const unsigned long hardware_threads = std::thread::hardware_concurrency();

  const unsigned long num_threads =
      std::min(max_threads, std::max(2ul, hardware_threads));

  const unsigned long block_size = length / num_threads;

  // ���տ��С�첽ִ��
  // ������ռ���Է�ֹ����push_back�׳��쳣
  std::vector<std::future<void>> fut_list(num_threads - 1);
  std::vector<std::thread> threads(num_threads - 1);
  ThreadingGuard thd_guard(threads);

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

  // ������ȡ���
  for (auto& fut : fut_list) {
    fut.get();
  }
}
}  // namespace playground::parallel
#endif
