#ifndef PLAYGROUND_THREADING_THREADSAFE_QUEUE_H_
#define PLAYGROUND_THREADING_THREADSAFE_QUEUE_H_

namespace playground {
template <typename T>
class ThreadsafeQueue {
 public:
  ThreadsafeQueue() : head_(std::make_unique<Node>()), tail_(head_.get()) {}
  ThreadsafeQueue(const ThreadsafeQueue&) = delete;
  ThreadsafeQueue& operator()(const ThreadsafeQueue&) = delete;

  void push(T new_value) {
    auto new_tail = std::make_unique<Node>();
    auto new_data = std::make_shared<T>(std::move(new_value));

    {
      std::lock_guard lock(tail_mtx_);
      tail_->data_ = new_data;
      tail_->next_ = std::move(new_tail);
      tail_ = tail_->next_.get();
    }
    cv_.notify_one();
  }

  bool try_pop(T& value) {
    std::lock_guard lock(head_mtx_);
    if (head_.get() == get_tail()) {
      return false;
    }
    auto old_head = pop_head();
    value = std::move(*old_head->data_);
    return true;
  }

  std::shared_ptr<T> try_pop() {
    std::lock_guard lock(head_mtx_);
    if (head_.get() == get_tail()) {
      return false;
    }
    auto old_head = pop_head();
    return old_head->data_;
  }

  void wait_and_pop(T& value) {
    std::unique_lock head_lock(wait_for_data());
    auto old_head = pop_head();
    value = std::move(*old_head->data_);
  }

  std::shared_ptr<T> wait_and_pop() {
    std::unique_lock head_lock(wait_for_data());
    auto old_head = pop_head();
    return old_head->data_;
  }

  bool empty() const {
    std::lock_guard head_lock(head_mtx_);
    return head_.get() == get_tail();
  }

 private:
  struct Node {
    //Node(T data) {}

    std::shared_ptr<T> data_;
    std::unique_ptr<Node> next_;
  };

  Node* get_tail() const {
    std::lock_guard lock(tail_mtx_);
    return tail_;
  }

  std::unique_lock<std::mutex> wait_for_data() const {
    std::unique_lock head_lock(head_mtx_);
    cv_.wait(head_lock, [this] { head_.get() != get_tail(); });
    return head_lock;
  }

  std::unique_ptr<Node> pop_head() {
    auto old_head = std::move(head_);
    head_ = std::move(old_head->next_);
    return old_head;
  }

 private:
  std::unique_ptr<Node> head_;
  std::mutex head_mtx_;
  Node* tail_;
  mutable std::mutex tail_mtx_;
  std::condition_variable cv_;
};
}  // namespace playground
#endif
