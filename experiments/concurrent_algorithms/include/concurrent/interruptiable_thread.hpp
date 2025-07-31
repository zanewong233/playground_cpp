#ifndef PLAYGROUND_EXPERIMENTS_CONCURRENT_INTERRUPTIABLE_THREAD_H_
#define PLAYGROUND_EXPERIMENTS_CONCURRENT_INTERRUPTIABLE_THREAD_H_
#include <future>
#include <thread>

namespace playground::experiments::parallel {
class InterruptFlag {
 public:
  InterruptFlag() : flag_(false), cv_(nullptr) {}

  bool IsSet() const { return flag_.load(std::memory_order_relaxed); }

  void Set() {
    flag_.store(true, std::memory_order_relaxed);
    std::lock_guard lock(set_clear_mutex_);
    if (cv_) {
      cv_->notify_all();
    }
  }

  void SetConditionVariable(std::condition_variable* cv) {
    std::lock_guard lock(set_clear_mutex_);
    cv_ = cv;
  }

  void ClearConditionVariable() {
    std::lock_guard lock(set_clear_mutex_);
    cv_ = nullptr;
  }

  struct ConditionVariableGuard {
    InterruptFlag* flag_ = nullptr;
    ConditionVariableGuard(InterruptFlag* flag) : flag_(flag) {}
    ~ConditionVariableGuard() { flag_->ClearConditionVariable(); }
  };

 private:
  std::atomic_bool flag_;
  std::condition_variable* cv_;
  std::mutex set_clear_mutex_;
};
thread_local InterruptFlag this_thread_interrupt_flag;

void InterruptPoint() {
  if (this_thread_interrupt_flag.IsSet()) {
    throw std::runtime_error("interrupt point!");
  }
}

template <typename Predicate>
void InterruptWait(std::condition_variable& cv,
                   std::unique_lock<std::mutex>& lock, Predicate& pred) {
  InterruptPoint();
  this_thread_interrupt_flag.SetConditionVariable(&cv);
  InterruptFlag::ConditionVariableGuard guard(&this_thread_interrupt_flag);
  while (!this_thread_interrupt_flag.IsSet() && !pred()) {
    cv.wait_for(lock, std::chrono::milliseconds(1));
  }
  InterruptPoint();
}

class InterruptiableThread {
 public:
  template <typename F>
  InterruptiableThread(F&& f) {
    std::promise<InterruptFlag*> promise;
    thread_ = std::thread([f = std::forward<F>(f), &promise]() {
      promise.set_value(&this_thread_interrupt_flag);
      static_assert(std::is_invocable_v<F>, "F 必须可被无参调用");
      std::cout << typeid(F).name() << "\n";
      f();
    });
    flag_ = promise.get_future().get();
  }
  ~InterruptiableThread() {
    if (thread_.joinable()) {
      thread_.join();
    }
  }

  bool Joinable() const { return thread_.joinable(); }

  void Join() { thread_.join(); }

  void Detch() { thread_.detach(); }

  void Interrupt() {
    if (flag_) {
      flag_->Set();
    }
  }

 private:
  std::thread thread_;
  InterruptFlag* flag_;
};
}  // namespace playground::experiments::parallel
#endif
