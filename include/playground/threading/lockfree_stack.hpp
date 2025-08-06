#ifndef PLAYGROUND_THREADING_LOCKFREE_STACK_H_
#define PLAYGROUND_THREADING_LOCKFREE_STACK_H_
#include <memory>

namespace playground {
template <typename T>
class LockfreeStack {
 public:
  void Push(const T& data) {
    const auto new_node = new Node(data);
    new_node->next_ = head_.load();
    while (!head_.compare_exchange_weak(new_node->next_, new_node));
  }

  std::shared_ptr<T> Pop() {
    Node* old_head = head_.load();
    while (old_head && !head_.compare_exchange_weak(old_head, old_head->next_));
    std::shared_ptr<T> res;
    if (old_head) {
      res = old_head->data_;
      delete old_head;
    }
    return res;
  }

 private:
  struct Node {
    Node(const T& data) { data_ = std::make_shared<T>(data); }

    std::shared_ptr<T> data_;
    Node* next_;
  };
  std::atomic<Node*> head_;
};
}  // namespace playground

#endif
