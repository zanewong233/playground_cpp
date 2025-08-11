/* 
* �ڸ����õ������ɾ���б���ܻ�һֱ����
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
    new_node->next_ = head_.load();
    while (!head_.compare_exchange_weak(new_node->next_, new_node));
  }

  std::shared_ptr<T> Pop() {
    ++thread_in_loop_;
    Node* old_head = head_.load();
    while (old_head && !head_.compare_exchange_weak(old_head, old_head->next_));

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
    if (thread_in_loop_.load() == 1) {
      // ֻ�е�ǰ�߳��ܿ���node
      auto tmp_to_delete = to_delete_list_.exchange(nullptr);
      if (--thread_in_loop_ == 0) {
        DeleteNodes(tmp_to_delete);
      } else if (tmp_to_delete) {
        ChainPendingNodes(tmp_to_delete);
      }

      delete node;
    } else {
      if (node) {
        ChainPendingNode(node);
      }

      --thread_in_loop_;
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
    last->next_ = to_delete_list_;
    while (!to_delete_list_.compare_exchange_weak(last->next_, first));
  }

  void ChainPendingNode(Node* node) { ChainPendingNodes(node, node); }

  std::atomic<Node*> head_;
  std::atomic_uint thread_in_loop_;
  std::atomic<Node*> to_delete_list_;
};
}  // namespace playground

#endif
