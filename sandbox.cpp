#include <chrono>
#include <future>
#include <list>
#include <mutex>
#include <numeric>
#include <thread>
#include <vector>

#include "playground/print_class/print_class.h"
#include "playground/threading/hierarchical_mutex.hpp"
#include "playground/threading/joining_thread.hpp"
#include "playground/threading/threadsafe_queue.hpp"
#include "playground/threading/threadsafe_stack.hpp"

using namespace playground;

template <typename T>
std::list<T> SequenceQuickSort(std::list<T>&& input) {
  if (input.size() <= 1) {
    return input;
  }

  T pivot = std::move(*input.begin());
  input.pop_front();

  std::list<T> lower, higher;
  for (auto it = input.begin(); it != input.end();) {
    auto cur = it++;
    if (*cur < pivot) {
      lower.splice(lower.end(), input, cur);
    } else {
      higher.splice(higher.end(), input, cur);
    }
  }

  lower = SequenceQuickSort(std::move(lower));
  higher = SequenceQuickSort(std::move(higher));

  lower.push_back(std::move(pivot));
  lower.splice(lower.end(), higher);
  return lower;
}

template <typename T>
std::list<T> ParallelQuickSort(std::list<T>&& input) {
  if (input.size() <= 1) {
    return input;
  }
  T pivot = std::move(*input.begin());
  input.pop_front();

  std::list<T> lower, higher;
  for (auto it = input.begin(); it != input.end();) {
    auto cur = it++;
    if (*cur < pivot) {
      lower.splice(lower.end(), input, cur);
    } else {
      higher.splice(higher.end(), input, cur);
    }
  }

  auto fut = std::async(&ParallelQuickSort<T>, std::move(lower));
  higher = ParallelQuickSort(std::move(higher));

  lower = fut.get();
  lower.push_back(std::move(pivot));
  lower.splice(lower.end(), higher);
  return lower;
}

int main() {
  std::list<int> list{5, 6, 7, 1, 4, 54, 48};
  auto sorted = ParallelQuickSort(std::move(list));

  int a = 10;
  a++;

  return 0;
}