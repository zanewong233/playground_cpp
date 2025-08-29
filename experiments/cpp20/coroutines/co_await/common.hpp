#ifndef PLAYGROUND_EXPERIMENTS_CPP20_COROUTINES_CO_AWAIT_COMMON_HPP_
#define PLAYGROUND_EXPERIMENTS_CPP20_COROUTINES_CO_AWAIT_COMMON_HPP_
#include <coroutine>
#include <iostream>
#include <thread>

#include "playground/base/macros.hpp"

namespace common {
struct Task {
  struct promise_type {
    Task get_return_object() {
      return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::suspend_always initial_suspend() noexcept {
      LOG_FUNC();
      return {};
    }
    std::suspend_always final_suspend() noexcept {
      LOG_FUNC();
      return {};
    }
    void return_void() noexcept { LOG_FUNC(); }
    void unhandled_exception() { std::terminate(); }
  };

  using handle = std::coroutine_handle<promise_type>;
  handle h_;
  explicit Task(handle h) : h_(h) { LOG_FUNC(); }
  ~Task() {
    if (h_) {
      h_.destroy();
    }
  }

  void run_to_end() {
    while (!h_.done()) {
      h_.resume();
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

  void resume() {
    if (h_) {
      h_.resume();
    }
  }
};
}  // namespace common
#endif
