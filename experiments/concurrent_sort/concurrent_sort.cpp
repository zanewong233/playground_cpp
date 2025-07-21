#include <list>
#include <future>

#include "playground/threading/threadsafe_stack.hpp"

using namespace playground;

template <typename T>
class Sorter {
 public:
  Sorter()
      : max_thread_count_(std::thread::hardware_concurrency() - 1),
        end_of_data_(false) {}
  ~Sorter() {
    end_of_data_ = true;
    for (int i = 0; i < threads_.size(); i++) {
      threads_[i].join();
    }
  }

  std::list<T> DoSort(std::list<T> list) {
    if (list.size() < 2) {
      return list;
    }
    // 取出中间点
    std::list<T> res;
    res.splice(res.end(), list, list.begin());
    const T& divide_point = *res.begin();

    // 将list 按照中间点分区
    const auto divide_it = std::partition(
        list.begin(), list.end(),
        [&divide_point](const T& val) { return val < divide_point; });

    // 前半部分排序
    ChunkToSort lower_chunk;
    lower_chunk.data_.splice(lower_chunk.data_.begin(), list, list.begin(),
                             divide_it);
    std::future<std::list<T>> lower_fut = lower_chunk.prms_.get_future();
    chunks_.Push(std::move(lower_chunk));
    if (threads_.size() < max_thread_count_) {
      threads_.emplace_back(&Sorter::SortThread, this);
    }

    // 后半部分排序
    std::list<T> sorted_higher = DoSort(list);
    res.splice(res.end(), sorted_higher);

    // 任务窃取
    while (lower_fut.wait_for(std::chrono::seconds(0)) !=
           std::future_status::ready) {
      TrySortChunk();
    }
    res.splice(res.begin(), lower_fut.get());
    return res;
  }

 private:
  void SortThread() {
    while (!end_of_data_) {
      TrySortChunk();
      std::this_thread::yield();
    }
  }

  void TrySortChunk() {
    if (auto chunk = chunks_.Pop()) {
      chunk->prms_.set_value(DoSort(chunk->data_));
    }
  }

 private:
  struct ChunkToSort {
    std::list<T> data_;
    std::promise<std::list<T>> prms_;
  };

  ThreadsafeStack<ChunkToSort> chunks_;
  std::vector<std::thread> threads_;
  const unsigned max_thread_count_;
  std::atomic<bool> end_of_data_;
};

void func() {
  std::list<int> vals{13, 7,  1,  20, 9, 4,  17, 3,  15, 6,
                      2,  11, 18, 14, 5, 10, 8,  19, 16, 12};
  Sorter<int> s;
  auto sorted = s.DoSort(vals);

  int a = 10;
  a++;
}

void func1() {
  int a = 10;
  a++;
}

int main() {
  func();
  func1();

  int a = 10;
  a++;

  return 0;
}
