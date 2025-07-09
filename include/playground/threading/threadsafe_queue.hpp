#ifndef PLAYGROUND_THREADING_THREADSAFE_QUEUE_H_
#define PLAYGROUND_THREADING_THREADSAFE_QUEUE_H_

namespace playground {
template <typename T>
class ThreadsafeQueue {
 public:
  ThreadsafeQueue() : head_(std::make_unique<Node>(T{})), tail_(head_.get()) {}
  ThreadsafeQueue(const ThreadsafeQueue&) = delete;
  ThreadsafeQueue& operator()(const ThreadsafeQueue&) = delete;

  void push(T new_value) {
    auto new_tail = std::make_unique<Node>(T{});
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
    auto old_head = pop_head();
    if (!old_head) {
      return {};
    }
    value = std::move(*old_head->data_);
    return true;
  }

  std::shared_ptr<T> try_pop() {
    auto old_head = pop_head();
    return old_head ? old_head->data_ : std::shared_ptr<T>{};
  }

  std::shared_ptr<T> wait_and_pop() {
    std::unique_lock lock(head_mtx_);
    cv_.wait(lock, [this] { return head_.get() != get_tail(); });

    auto old_head = std::move(head_);
    head_ = old_head->next_;
    return old_head->data_;
  }

 private:
  struct Node {
    Node(T data) {}

    std::shared_ptr<T> data_;
    std::unique_ptr<Node> next_;
  };

  Node* get_tail() const {
    std::lock_guard lock(tail_mtx_);
    return tail_;
  }

  std::unique_ptr<Node> pop_head() {
    // ������get_tail֮ǰ��head_mtx_��������������������ֻ��tail���ܱ仯��
    // ���õ����Ǿ�ֵ������head ��tail ���ܶ��ı��ˣ����������Ԥ�⡣
    // Ҳ����˵head ��������֤�˳��������ڼ����ֻ�ܴ�һ�������޸ģ�
    // �����ǰ�ȫ�ġ�
    std::lock_guard lock(head_mtx_);
    if (head_.get() == get_tail()) {
      return {};
    }
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
#endif  // PLAYGROUND_THREADING_HIERARCHICAL_MUTEX_H_
