// 测试伪共享对于代码性能的影响
// 伪共享：不同线程的数据位于同一个缓存行，导致内核间需要同步数据，导致性能浪费
#include <atomic>
#include <chrono>
#include <iostream>
#include <new>
#include <thread>
#include <vector>

constexpr std::size_t CACHELINE = std::hardware_destructive_interference_size;
constexpr std::size_t ITERS = 50'000'000;
const unsigned THREADS = std::thread::hardware_concurrency();

struct NonPadded {
  std::atomic<uint64_t> v{0};
};

struct Padded {
  std::atomic<uint64_t> v{0};
  char padding[CACHELINE - sizeof(uint64_t)];
};

template <typename T>
std::chrono::microseconds bench() {
  std::vector<T> counters(THREADS);

  auto t0 = std::chrono::high_resolution_clock::now();

  std::vector<std::thread> threads;
  for (unsigned i = 0; i < THREADS; i++) {
    threads.emplace_back([&counters, i] {
      for (int n = 0; n < ITERS; ++n) {
        counters[i].v.fetch_add(1, std::memory_order_relaxed);
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  auto t1 = std::chrono::high_resolution_clock::now();
  return std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0);
}

int main() {
  auto t_shared = bench<NonPadded>();
  auto t_padded = bench<Padded>();

  std::cout << "Threads            : " << THREADS << '\n'
            << "Iterations/thread  : " << ITERS << '\n'
            << "Non-padded (ms)    : " << t_shared.count() << '\n'
            << "Padded    (ms)    : " << t_padded.count() << '\n'
            << "Speed-up           : "
            << static_cast<double>(t_shared.count()) / t_padded.count()
            << "×\n";
  return 0;
}
