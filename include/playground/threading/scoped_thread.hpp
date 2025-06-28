#pragma once
#include <thread>

namespace playground {
class ScopedThread {
 private:
  std::thread thread_;

 public:
  explicit ScopedThread(std::thread t) : thread_(std::move(t)) {
    if (!thread_.joinable()) {
      throw std::logic_error("Thread is not joinable");
    }
  }
  ~ScopedThread() { thread_.join(); }

  ScopedThread(const ScopedThread&) = delete;
  ScopedThread& operator=(const ScopedThread&) = delete;
};
}  // namespace playground
