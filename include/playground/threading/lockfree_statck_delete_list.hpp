/*
 * 在高争用的情况下删除列表可能会一直扩张
 */
#ifndef PLAYGROUND_THREADING_LOCKFREE_STACK_DELETE_LIST_H_
#define PLAYGROUND_THREADING_LOCKFREE_STACK_DELETE_LIST_H_
#include <memory>

namespace playground {
template <typename T>
class LockfreeStackDeleteList {
 public:
  LockfreeStackDeleteList()
      : head_(nullptr), thread_in_loop_(0), to_delete_list_(nullptr) {}

  void Push(const T& data) {
    const auto new_node = new Node(data);
    new_node->next_ = head_.load(std::memory_order_relaxed);
    while (!head_.compare_exchange_weak(new_node->next_, new_node,
                                        std::memory_order_release,
                                        std::memory_order_relaxed));
  }

  std::shared_ptr<T> Pop() {
    thread_in_loop_.fetch_add(1, std::memory_order_relaxed);
    Node* old_head = head_.load(std::memory_order_relaxed);
    while (old_head && !head_.compare_exchange_weak(old_head, old_head->next_,
                                                    std::memory_order_acquire,
                                                    std::memory_order_relaxed));

    std::shared_ptr<T> res;
    if (old_head) {
      res.swap(old_head->data_);
    }
    TryReclaim(old_head);
    return res;
  }

 private:
  struct Node {
    Node(const T& data) { data_ = std::make_shared<T>(data); }

    std::shared_ptr<T> data_;
    Node* next_;
  };

  void TryReclaim(Node* node) {
    if (thread_in_loop_.load(std::memory_order_relaxed) == 1) {
      // 只有当前线程能看到node
      auto tmp_to_delete =
          to_delete_list_.exchange(nullptr, std::memory_order_acquire);
      if (thread_in_loop_.fetch_sub(1, std::memory_order_relaxed) == 1) {
        DeleteNodes(tmp_to_delete);
      } else if (tmp_to_delete) {
        ChainPendingNodes(tmp_to_delete);
      }

      delete node;
    } else {
      if (node) {
        ChainPendingNode(node);
      }

      thread_in_loop_.fetch_sub(1, std::memory_order_relaxed);
    }
  }

  void DeleteNodes(Node* nodes) {
    auto current = nodes;
    while (current) {
      auto next = current->next_;
      delete current;
      current = next;
    }
  }

  void ChainPendingNodes(Node* nodes) {
    auto last = nodes;
    while (auto next = last->next_) {
      last = next;
    }
    ChainPendingNodes(nodes, last);
  }

  void ChainPendingNodes(Node* first, Node* last) {
    last->next_ = to_delete_list_.load(std::memory_order_relaxed);
    while (!to_delete_list_.compare_exchange_weak(last->next_, first,
                                                  std::memory_order_release,
                                                  std::memory_order_relaxed));
  }

  void ChainPendingNode(Node* node) { ChainPendingNodes(node, node); }

  std::atomic<Node*> head_;
  std::atomic_uint thread_in_loop_;
  std::atomic<Node*> to_delete_list_;
};
}  // namespace playground

#endif
