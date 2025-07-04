#ifndef PLAYGROUND_THREADING_THREADSAFE_QUEUE_H_
#define PLAYGROUND_THREADING_THREADSAFE_QUEUE_H_
#include <memory>
#include <mutex>
#include <queue>

namespace playground {
template <typename T>
class ThreadsafeQueue {
 public:
  explicit ThreadsafeQueue() = default;
  ThreadsafeQueue(const ThreadsafeQueue& other) {
    std::lock_guard lock(other.m_);
    data_ = other.data_;
  }

  void push(const T& val) {
    std::lock_guard lock(m_);
    data_.push(val);
    cv_.notify_one();
  }
  void push(T&& val) {
    std::lock_guard lock(m_);
    data_.push(std::move(val));
    cv_.notify_one();
  }

  void wait_and_pop(T& val) {
    std::unique_lock lock(m_);
    cv_.wait(lock, [this]() { return !data_.empty(); });
    val = std::move(data_.front());
    data_.pop();
  }
  std::shared_ptr<T> wait_and_pop() {
    std::unique_lock lock(m_);
    cv_.wait(lock, [this]() { return !data_.empty(); });
    std::shared_ptr<T> res = std::make_shared<T>(std::move(data_.front()));
    data_.pop();
    return res;
  }

  bool try_pop(T& val) {
    std::lock_guard lock(m_);
    if (data_.empty()) {
      return false;
    }
    val = std::move(data_.front());
    data_.pop();
    return true;
  }
  std::shared_ptr<T> try_pop() {
    std::lock_guard lock(m_);
    if (data_.empty()) {
      return {};
    }
    std::shared_ptr<T> res = std::make_shared<T>(std::move(data_.front()));
    data_.pop();
    return res;
  }

  bool empty() const {
    std::lock_guard lock(m_);
    return data_.empty();
  }

 private:
  std::mutex m_;
  std::condition_variable cv_;

  std::queue<T> data_;
};
}  // namespace playground
#endif  // PLAYGROUND_THREADING_HIERARCHICAL_MUTEX_H_
