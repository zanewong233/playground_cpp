#pragma once
#include <mutex>
#include <numeric>
#include <stack>
#include <thread>
#include <vector>

namespace playground {
struct EmptyStack : public std::exception {
  const char* what() const noexcept override { return "Empty stack!"; }
};

template <typename T>
class ThreadsafeStack {
  std::stack<T> data_;
  std::mutex m_;

 public:
  ThreadsafeStack() : data_(std::stack<T>()) {}
  ThreadsafeStack(const ThreadsafeStack& other) {
    std::lock_guard lock(other.m_);
    data_ = other.data_;
  }
  ThreadsafeStack& operator=(const ThreadsafeStack&) = delete;

  void Push(const T& new_value) {
    std::lock_guard lock(m_);
    data_.push(new_value);
  }

  void Pop(T& value) {
    T tmp;
    {
      std::lock_guard lock(m_);
      if (data_.empty()) throw EmptyStack{};
      tmp = std::move(data_.top());
      data_.pop();
    }
    value = std::move(value);
  }

  std::shared_ptr<T> Pop() {
    T tmp;
    {
      std::lock_guard lock(m_);
      if (data_.empty()) throw EmptyStack{};
      tmp = std::move(data_.top());
      data_.pop();
    }
    return std::make_shared<T>(std::move(tmp));
  }
};

}  // namespace playground
