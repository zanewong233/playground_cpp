/*
 * 实现较为复杂，可能会略微拖慢效率。该方法也可能存在专利授权问题
 */
#ifndef PLAYGROUND_THREADING_LOCKFREE_STACK_HAZARD_POINTER_H_
#define PLAYGROUND_THREADING_LOCKFREE_STACK_HAZARD_POINTER_H_
#include <memory>

namespace playground {
struct HazardPointer {
  std::atomic<std::thread::id> thread_id_ = std::thread::id();
  std::atomic<void*> hp_ = nullptr;
};

constexpr unsigned max_hazard_pointers = 100;
HazardPointer hazard_pointers[max_hazard_pointers];

class HpOwner {
 public:
  HpOwner() : p_(nullptr) {
    for (int i = 0; i < max_hazard_pointers; i++) {
      std::thread::id init_id;
      if (hazard_pointers[i].thread_id_.compare_exchange_strong(
              init_id, std::this_thread::get_id())) {
        p_ = &hazard_pointers[i];
        break;
      }
    }

    if (p_ == nullptr) {
      throw std::runtime_error("cannot get hazard pointer!");
    }
  }
  ~HpOwner() {
    p_->hp_.store(nullptr);
    p_->thread_id_.store({});
  }

  std::atomic<void*>& GetPointer() const { return p_->hp_; }

 private:
  HazardPointer* p_;
};

template <typename T>
void DoDelete(void* p) {
  delete static_cast<T*>(p);
}

struct DataToReclaim {
  template <typename T>
  DataToReclaim(T* data)
      : data_(data), deleter_(&DoDelete<T>), next_(nullptr) {}
  ~DataToReclaim() { deleter_(data_); }

  void* data_;
  std::function<void(void*)> deleter_;
  DataToReclaim* next_;
};

template <typename T>
class LockfreeStackHazardPointer {
 public:
  LockfreeStackHazardPointer() : head_(nullptr), nodes_to_reclaim_(nullptr) {}

  void Push(const T& data) {
    const auto new_node = new Node(data);
    new_node->next_ = head_.load();
    while (!head_.compare_exchange_weak(new_node->next_, new_node));
  }

  std::shared_ptr<T> Pop() {
    std::atomic<void*>& hp = GetHazardPointerForCurrentThread();
    Node* old_head = head_.load();
    do {
      Node* tmp = nullptr;
      do {
        tmp = old_head;
        hp.store(old_head);
        old_head = head_.load();
      } while (tmp != old_head);
    } while (old_head &&
             !head_.compare_exchange_strong(old_head, old_head->next_));
    hp.store(nullptr);

    std::shared_ptr<T> res;
    if (old_head) {
      res.swap(old_head->data_);
      if (OutstandingHazardPointersFor(old_head)) {
        ReclaimLater(old_head);
      } else {
        delete old_head;
      }
      DeleteNodesWithNoHazards();
    }

    return res;
  }

 private:
  struct Node {
    Node(const T& data) { data_ = std::make_shared<T>(data); }

    std::shared_ptr<T> data_;
    Node* next_;
  };

  std::atomic<void*>& GetHazardPointerForCurrentThread() {
    static thread_local HpOwner hp_owner;
    return hp_owner.GetPointer();
  }

  bool OutstandingHazardPointersFor(void* node) {
    for (int i = 0; i < max_hazard_pointers; i++) {
      if (hazard_pointers[i].hp_.load() == node) {
        return true;
      }
    }
    return false;
  }

  void ReclaimLater(Node* node) { AddToReclaimList(new DataToReclaim(node)); }

  void DeleteNodesWithNoHazards() {
    auto current = nodes_to_reclaim_.exchange(nullptr);
    while (current) {
      const auto next = current->next_;
      if (OutstandingHazardPointersFor(current->data_)) {
        AddToReclaimList(current);
      } else {
        delete current;
      }
      current = next;
    }
  }

  void AddToReclaimList(DataToReclaim* node) {
    node->next_ = nodes_to_reclaim_.load();
    while (!nodes_to_reclaim_.compare_exchange_weak(node->next_, node));
  }

  std::atomic<Node*> head_;
  std::atomic<DataToReclaim*> nodes_to_reclaim_;
};
}  // namespace playground

#endif
