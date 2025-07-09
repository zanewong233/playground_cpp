#ifndef PLAYGROUND_THREADING_THREADSAFE_QUEUE_H_
#define PLAYGROUND_THREADING_THREADSAFE_QUEUE_H_
#include <memory>

namespace playground {
template <typename T>
class SimpleQueue {
 public:
  SimpleQueue() : head_(std::make_unique<Node>(T{})), tail_(head_.get()) {}
  SimpleQueue(const SimpleQueue&) = delete;
  SimpleQueue& operator()(const SimpleQueue&) = delete;

  void push(T new_value) {
    tail_->data_ = std::make_shared<T>(std::move(new_value));
    std::unique_ptr new_tail = std::make_unique<Node>(T{});
    tail_->next_ = std::move(new_tail);
    tail_ = tail_->next_.get();
  }

  bool try_pop(T& value) {
    if (head_.get() == tail_) {
      return false;
    }

    value = std::move(*head_->data_);
    head_ = std::move(head_->next_);
    return true;
  }

  std::shared_ptr<T> try_pop() {
    if (head_.get() == tail_) {
      return {};
    }
    auto res = head_->data_;
    head_ = std::move(head_->next_);
    return res;
  }

 private:
  struct Node {
    Node(T data) {}

    std::shared_ptr<T> data_;
    std::unique_ptr<Node> next_;
  };

  std::unique_ptr<Node> head_;
  Node* tail_;
};
}  // namespace playground
#endif  // PLAYGROUND_THREADING_HIERARCHICAL_MUTEX_H_
