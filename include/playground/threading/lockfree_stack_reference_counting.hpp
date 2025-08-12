#ifndef PLAYGROUND_THREADING_LOCKFREE_STACK_REFERENCE_COUNTING_H_
#define PLAYGROUND_THREADING_LOCKFREE_STACK_REFERENCE_COUNTING_H_
#include <memory>

namespace playground {
template <typename T>
class LockfreeStackReferenceCounting {
 public:
  LockfreeStackReferenceCounting() = default;
  ~LockfreeStackReferenceCounting() { while (Pop()); }

  void Push(const T& data) {
    CountedNodePtr new_node;
    new_node.node_ = new Node(data);
    new_node.external_count_ = 1;
    new_node.node_->next_ = head_.load(std::memory_order_relaxed);
    while (!head_.compare_exchange_weak(new_node.node_->next_, new_node,
                                        std::memory_order_release,
                                        std::memory_order_relaxed));
  }

  std::shared_ptr<T> Pop() {
    while (true) {
      CountedNodePtr old_head = head_.load(std::memory_order_relaxed);
      IncreaseHeadCount(old_head);
      Node* const node = old_head.node_;
      if (!node) {
        return {};
      }

      if (head_.compare_exchange_strong(old_head, node->next_,
                                        std::memory_order_relaxed)) {
        std::shared_ptr<T> res;
        res.swap(node->data_);
        const int count_increase = old_head.external_count_ - 2;
        if (node->internal_count_.fetch_add(
                count_increase, std::memory_order_release) == -count_increase) {
          delete node;
        }
        return res;
      } else {
        if (node->internal_count_.fetch_sub(1, std::memory_order_relaxed) ==
            1) {
          node->internal_count_.load(std::memory_order_acquire);
          delete node;
        }
      }
    }
  }

 private:
  struct Node;
  struct CountedNodePtr {
    Node* node_ = nullptr;
    unsigned external_count_ = 0;  // 外部引用计数：有多少线程引用了本节点
  };

  struct Node {
    Node(const T& data)
        : data_(std::make_shared<T>(data)), internal_count_(0) {}

    std::shared_ptr<T> data_;
    CountedNodePtr next_;
    std::atomic_uint
        internal_count_;  // 内部引用计数：有多少线程归还了本节点的引用
  };

  void IncreaseHeadCount(CountedNodePtr& old_head) {
    CountedNodePtr new_head;
    do {
      new_head = old_head;
      ++new_head.external_count_;
    } while (!head_.compare_exchange_strong(old_head, new_head,
                                            std::memory_order_acquire,
                                            std::memory_order_relaxed));
    old_head.external_count_ = new_head.external_count_;
  }

  std::atomic<CountedNodePtr> head_;
};
}  // namespace playground

#endif
