#ifndef PLAYGROUND_EXPERIMENTS_CONCURRENT_SORT_H_
#define PLAYGROUND_EXPERIMENTS_CONCURRENT_SORT_H_
#include <future>
#include <list>

#include "playground/threading/threadsafe_stack.hpp"
#include "thread_pool.hpp"

namespace playground::experiments::parallel {
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

  std::list<T> DoSort(std::list<T>& list) {
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
    std::list<T> sorted_higher = DoSort(std::move(list));
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
      chunk->prms_.set_value(DoSort(std::move(chunk->data_)));
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

template <typename T>
class SorterThreadPool {
 public:
  std::list<T> DoSort(std::list<T> list) {
    if (list.size() < 2) {
      return list;
    }
    // 取出中间点
    MoveMedianOfThree2Begin(list);
    std::list<T> res;
    res.splice(res.end(), list, list.begin());
    const T& divide_point = *res.begin();

    // 将list 按照中间点分区
    const auto divide_it = std::partition(
        list.begin(), list.end(),
        [&divide_point](const T& val) { return val < divide_point; });

    // 前半部分排序
    std::list<T> lower_chunk;
    lower_chunk.splice(lower_chunk.begin(), list, list.begin(), divide_it);
    auto lower_fut = pool_.AddTask(
        std::bind(&SorterThreadPool::DoSort, this, std::move(lower_chunk)));

    // 后半部分排序
    std::list<T> sorted_higher = DoSort(std::move(list));
    res.splice(res.end(), sorted_higher);

    // 任务窃取
    while (lower_fut.wait_for(std::chrono::seconds(0)) !=
           std::future_status::ready) {
      pool_.RunPendingTasks();
    }
    res.splice(res.begin(), lower_fut.get());
    return res;
  }

 private:
  void MoveMedianOfThree2Begin(std::list<T>& list) {
    auto length = std::distance(list.begin(), list.end());
    if (length < 3) {
      return;
    }

    auto left_it = list.begin();
    auto right_it = list.end();
    --right_it;
    auto mid_it = left_it;
    std::advance(mid_it, length / 2);

    if (*left_it > *mid_it) {
      std::iter_swap(left_it, mid_it);
    }
    if (*left_it > *right_it) {
      std::iter_swap(left_it, right_it);
    }
    if (*mid_it > *right_it) {
      std::iter_swap(mid_it, right_it);
    }
    std::iter_swap(left_it, mid_it);  // 移动到开始位置
  }

  ThreadPool pool_;
};

}  // namespace playground::experiments::parallel
#endif
