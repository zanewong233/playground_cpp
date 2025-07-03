// 提供一个层级锁，持有高层级的锁时可以对低层级的锁进行加锁
// 主要用于解决多个锁加锁顺序不一致导致的死锁问题

#ifndef PLAYGROUND_THREADING_HIERARCHICAL_MUTEX_H_
#define PLAYGROUND_THREADING_HIERARCHICAL_MUTEX_H_
#include <mutex>

namespace playground {
class HierarchicalMutex {
 public:
  explicit HierarchicalMutex(unsigned long value)
      : hierarchy_value_(value), previous_hierarchy_value_(0) {}

  void lock() {
    check_for_hierarchy_violation();
    m_.lock();
    update_hierarchy_value();
  }

  void unlock() {
    if (this_thread_hierarchy_value_ != hierarchy_value_) {
      throw std::logic_error("mutex hierarchy violated!");
    }
    this_thread_hierarchy_value_ = previous_hierarchy_value_;
    m_.unlock();
  }

  bool try_lock() {
    check_for_hierarchy_violation();
    if (!m_.try_lock()) {
      return false;
    }
    update_hierarchy_value();
    return true;
  }

 private:
  void check_for_hierarchy_violation() const {
    if (this_thread_hierarchy_value_ <= hierarchy_value_) {
      throw std::logic_error("mutex hierarchy violated!");
    }
  }

  void update_hierarchy_value() {
    previous_hierarchy_value_ = this_thread_hierarchy_value_;
    this_thread_hierarchy_value_ = hierarchy_value_;
  }

 private:
  std::mutex m_;

  const unsigned long hierarchy_value_;
  unsigned long previous_hierarchy_value_;
  inline thread_local static unsigned long this_thread_hierarchy_value_ =
      ULONG_MAX;
};
}  // namespace playground
#endif
