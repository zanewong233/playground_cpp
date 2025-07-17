#ifndef PLAYGROUND_THREADING_THREADSAFE_STACK_H_
#define PLAYGROUND_THREADING_THREADSAFE_STACK_H_
#include <mutex>
#include <numeric>
#include <stack>
#include <thread>
#include <vector>

namespace playground {
template <typename T>
class ThreadsafeStack {
 public:
  ThreadsafeStack() : data_(std::stack<T>()) {}
  ThreadsafeStack(const ThreadsafeStack& other) {
    std::lock_guard lock(other.m_);
    data_ = other.data_;
  }
  ThreadsafeStack& operator=(const ThreadsafeStack&) = delete;

  void Push(T new_value) {
    std::lock_guard lock(m_);
    data_.push(std::move(new_value));
  }

  bool Pop(T& value) {
    T tmp;
    {
      std::lock_guard lock(m_);
      if (data_.empty()) {
        return false;
      }
      tmp = std::move(data_.top());
      data_.pop();
    }
    value = std::move(value);
    return true;
  }

  std::shared_ptr<T> Pop() {
    T tmp;
    {
      std::lock_guard lock(m_);
      if (data_.empty()) {
        return {};
      }
      tmp = std::move(data_.top());
      data_.pop();
    }
    return std::make_shared<T>(std::move(tmp));
  }

  size_t Size() const {
    std::lock_guard lock(m_);
    return data_.size();
  }

 private:
  std::stack<T> data_;
  mutable std::mutex m_;
};
}  // namespace playground
#endif
