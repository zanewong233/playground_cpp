#ifndef PLAYGROUND_THREADING_THREADSAFE_LIST_H_
#define PLAYGROUND_THREADING_THREADSAFE_LIST_H_
#include <memory>
#include <mutex>

namespace playground {
template <typename T>
class ThreadsafeList {
 public:
  ThreadsafeList() = default;
  ~ThreadsafeList() {}

  ThreadsafeList(const ThreadsafeList&) = delete;
  ThreadsafeList& operator=(const ThreadsafeList&) = delete;

  void PushFront(const T& value) {
    std::unique_ptr<Node> new_node = std::make_unique<Node>(value);
    std::lock_guard lk(head_.mtx_);
    new_node->next_ = std::move(head_.next_);
    head_.next_ = std::move(new_node);
  }

  template <typename Function>
  void ForEach(Function f) {
    std::unique_lock lk(head_.mtx_);
    const Node* current = &head_;
    while (const Node* next = current->next_.get()) {
      std::unique_lock next_lk(next->mtx_);
      lk.unlock();
      f(next->data_);
      current = next;
      lk = std::move(next_lk);
    }
  }

  template <typename Predicate>
  std::shared_ptr<T> FindFirstIf(Predicate p) {
    const Node* current = &head_;
    std::unique_lock lk(head_.mtx_);
    while (const Node* next = current->next_.get()) {
      std::unique_lock next_lk(next->mtx_);
      lk.unlock();
      if (p(next->data_)) {
        return next->data_;
      }
      current = next;
      lk = std::move(next_lk);
    }
    return {};
  }

  template <typename Predicate>
  void RemoveIf(Predicate p) {
    Node* current = &head_;
    std::unique_lock lk(head_.mtx_);
    while (Node* next = current->next_.get()) {
      std::unique_lock next_lk(next->mtx_);
      if (p(next->data_)) {
        std::unique_ptr old_next = std::move(current->next_);
        current->next_ = std::move(next->next_);
        next_lk.unlock();
      } else {
        lk.unlock();
        current = next;
        lk = std::move(next_lk);
      }
    }
  }

 private:
  struct Node {
    Node() = default;
    Node(const T& value) : data_(std::make_shared<T>(value)) {}

    std::shared_ptr<T> data_;
    std::unique_ptr<Node> next_;
    mutable std::mutex mtx_;
  };

  Node head_;
};
}  // namespace playground
#endif
