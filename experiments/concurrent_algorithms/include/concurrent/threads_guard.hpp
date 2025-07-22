#ifndef PLAYGROUND_EXPERIMENTS_CONCURRENT_THREADS_GUARD_H_
#define PLAYGROUND_EXPERIMENTS_CONCURRENT_THREADS_GUARD_H_
#include <thread>
#include <vector>

namespace playground::parallel {
class ThreadsGuard {
 public:
  ThreadsGuard(std::vector<std::thread>& ths) : threads_(ths) {}
  ~ThreadsGuard() {
    for (auto& it : threads_) {
      if (it.joinable()) {
        it.join();
      }
    }
  }

  ThreadsGuard(const ThreadsGuard&) = delete;
  ThreadsGuard& operator=(const ThreadsGuard&) = delete;

 private:
  std::vector<std::thread>& threads_;
};
}  // namespace playground::parallel
#endif
