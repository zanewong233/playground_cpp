#pragma once
#include <thread>

namespace playground {
class JoiningThread {
  std::thread t_;

 public:
  JoiningThread() noexcept = default;
  template <typename Callable, typename... Args>
  explicit JoiningThread(Callable&& f, Args&&... args)
      : t_(std::forward<Callable>(f), std::forward<Args>(args)...) {}
  explicit JoiningThread(std::thread&& t) noexcept : t_(std::move(t)) {}
  JoiningThread(JoiningThread&& other) noexcept : t_(std::move(other.t_)) {}
  JoiningThread& operator=(JoiningThread&& other) noexcept {
    if (Joinable()) {
      Join();
    }
    t_ = std::move(other.t_);
    return *this;
  }
  JoiningThread& operator=(std::thread&& t) noexcept {
    if (Joinable()) {
      Join();
    }
    t_ = std::move(t);
    return *this;
  }
  ~JoiningThread() noexcept {
    if (Joinable()) {
      Join();
    }
  }

  void Swap(JoiningThread& other) noexcept { t_.swap(other.t_); }

  std::thread::id GetId() const noexcept { return t_.get_id(); }

  std::thread& AsThread() noexcept { return t_; }
  const std::thread& AsThread() const noexcept { return t_; }

  bool Joinable() const noexcept { return t_.joinable(); }

  void Join() { t_.join(); }
  void Detach() { t_.detach(); }
};
}  // namespace playground
