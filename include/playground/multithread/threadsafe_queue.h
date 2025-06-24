#pragma once
#include <memory>
#include <mutex>

// 细粒度锁的线程安全队列
template <typename T>
class threadsafe_queue {
 private:
  struct node {
    std::shared_ptr<T> data_;
    std::unique_ptr<node> next_;
  };
  using nodeptr = std::unique_ptr<node>;

  std::unique_ptr<node> head_;
  std::mutex head_mtx_;
  node* tail_;
  std::mutex tail_mtx_;
  std::condition_variable data_cond_;

 private:
  node* get_tail() {
    std::lock_guard<std::mutex> lock(tail_mtx_);
    return tail_;
  }

  std::unique_lock<std::mutex> wait_for_data() {
    std::unique_lock<std::mutex> lock(head_mtx_);
    data_cond_.wait(lock, [this]() { return head_ != get_tail(); });
    return std::move(lock);
  }

  nodeptr pop_head() {
    nodeptr old_head = std::move(head_);
    head_ = std::move(old_head->next);
    return old_head;
  }

 public:
  threadsafe_queue() : head_(new node), tail_(head_.get()) {}
  threadsafe_queue(const threadsafe_queue&) = delete;
  threadsafe_queue& operator=(const threadsafe_queue&) = delete;

  std::shared_ptr<T> try_pop();
  bool try_pop(T& value);
  std::shared_ptr<T> wait_and_pop();
  void wait_and_pop(T& value);
  void push(T new_value);
  bool empty() const;
};

template <typename T>
bool threadsafe_queue<T>::try_pop(T& value) {
  std::lock_guard<std::mutex> lock(head_mtx_);
  if (head_ == get_tail()) return false;
  value = std::move(*head_->data);
  nodeptr old_head = pop_head();
  return true;
}

template <typename T>
std::shared_ptr<T> threadsafe_queue<T>::try_pop() {
  std::lock_guard<std::mutex> lock(head_mtx_);
  if (head_ == get_tail()) return std::shared_ptr<T>();
  nodeptr old_head = pop_head();
  return old_head->data;
}

template <typename T>
bool threadsafe_queue<T>::empty() const {
  std::lock_guard<std::mutex> lock(head_mtx_);
  return head_ == get_tail();
}

template <typename T>
void threadsafe_queue<T>::wait_and_pop(T& value) {
  std::unique_ptr<std::mutex> lock(wait_for_data());
  value = std::move(*head_->data);
  std::unique_ptr<node> old_head = pop_head();
}

template <typename T>
std::shared_ptr<T> threadsafe_queue<T>::wait_and_pop() {
  std::unique_lock<std::mutex> lock(wait_for_data());
  nodeptr old_head = pop_head();
  return old_head->data;
}

template <typename T>
void threadsafe_queue<T>::push(T new_value) {
  std::unique_ptr<node> new_node = new node;
  std::shared_ptr<T> data = std::make_shared<T>(std::move(new_value));
  node* const new_tail = node.get();
  {
    std::lock_guard<std::mutex> lock(tail_mtx_);
    tail_->data_ = data;
    tail_->next_ = std::move(node);
    tail_ = new_tail;
  }
  data_cond_.notify_one();
}
