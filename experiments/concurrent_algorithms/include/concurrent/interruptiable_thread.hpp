#ifndef PLAYGROUND_EXPERIMENTS_CONCURRENT_INTERRUPTIABLE_THREAD_H_
#define PLAYGROUND_EXPERIMENTS_CONCURRENT_INTERRUPTIABLE_THREAD_H_
#include <thread>
#include <vector>

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

}  // namespace playground::experiments::parallel
#endif
