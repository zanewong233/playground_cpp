/*
 * 使用这种实现方式的stack，需要先调用
 * SharedPtrAtomicLockFreeRuntimeProbe判断智能指针是否为lock-free 的
 */
#ifndef PLAYGROUND_THREADING_LOCKFREE_STACK_SHARED_PTR_H_
#define PLAYGROUND_THREADING_LOCKFREE_STACK_SHARED_PTR_H_
#include <memory>

namespace playground {
template <typename T>
class LockfreeStackSharedPtr {
 public:
  static bool SharedPtrAtomicLockFreeRuntimeProbe() {
    std::shared_ptr<T> tmp;
    return std::atomic_is_lock_free(&tmp);
  }

  LockfreeStackSharedPtr() = default;
  ~LockfreeStackSharedPtr() { while (Pop()); }

  void Push(const T& data) {
    auto new_node = std::make_shared<Node>(data);
    new_node->next_ = std::atomic_load(&head_);
    while (
        !std::atomic_compare_exchange_weak(&head_, &new_node->next_, new_node));
  }

  std::shared_ptr<T> Pop() {
    auto old_head = std::atomic_load(&head_);
    while (old_head &&
           !std::atomic_compare_exchange_weak(
               &head_, &old_head, std::atomic_load(&old_head->next_)));
    std::shared_ptr<T> res;
    if (old_head) {
      std::atomic_store(&old_head->next_, std::shared_ptr<Node>{});
      res = old_head->data_;
    }
    return res;
  }

 private:
  struct Node {
    Node(const T& data) : data_(std::make_shared<T>(data)) {}
    std::shared_ptr<T> data_;
    std::shared_ptr<Node> next_;
  };

  std::shared_ptr<Node> head_;
};
}  // namespace playground

#endif
