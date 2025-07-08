#ifndef PLAYGROUND_THREADING_THREADSAFE_QUEUE_H_
#define PLAYGROUND_THREADING_THREADSAFE_QUEUE_H_
#include <memory>

namespace playground {
template <typename T>
class SimpleQueue {
 public:
  SimpleQueue() : tail_(nullptr) {}
  SimpleQueue(const SimpleQueue&) = delete;
  SimpleQueue& operator()(const SimpleQueue&) = delete;

  void push(T new_value) {
    std::unique_ptr new_node(std::make_unique<Node>(std::move(new_value)));
    if (tail_) {
      tail_->next_ = std::move(new_node);
      tail_ = tail_->next_.get();
    } else {
      head_ = std::move(new_node);
      tail_ = head_.get();
    }
  }

  bool try_pop(T& value) {
    if (!head_) {
      return false;
    }

    value = std::move(head_->data_);
    head_ = std::move(head_->next_);
    if (!head_.get()) {
      tail_ = nullptr;
    }
    return true;
  }

 private:
  struct Node {
    Node(T data) : data_(std::move(data)) {}

    T data_;
    std::unique_ptr<Node> next_;
  };

  std::unique_ptr<Node> head_;
  Node* tail_;
};
}  // namespace playground
#endif  // PLAYGROUND_THREADING_HIERARCHICAL_MUTEX_H_
