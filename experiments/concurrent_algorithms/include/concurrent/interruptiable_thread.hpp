#ifndef PLAYGROUND_EXPERIMENTS_CONCURRENT_INTERRUPTIABLE_THREAD_H_
#define PLAYGROUND_EXPERIMENTS_CONCURRENT_INTERRUPTIABLE_THREAD_H_
#include <future>
#include <thread>

namespace playground::experiments::parallel {
class InterruptFlag {
 public:
  InterruptFlag() : is_setted_(false) {}
  void Set() { is_setted_ = true; }
  bool IsSet() const { return is_setted_; }

 private:
  bool is_setted_;
};
static InterruptFlag this_thread_interrupt_flag;

class InterruptiableThread {
 public:
  template <typename F>
  InterruptiableThread(F&& f) {
    std::promise<InterruptFlag*> promise;
    thread_ = std::thread([f = std::forward<F>(f), &promise]() {
      promise.set_value(&this_thread_interrupt_flag);
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

  void InterruptPoint() const {
    if (flag_->IsSet()) {
      throw std::runtime_error("interrupt point!");
    }
  }

 private:
  std::thread thread_;
  InterruptFlag* flag_;
};
}  // namespace playground::experiments::parallel
#endif
