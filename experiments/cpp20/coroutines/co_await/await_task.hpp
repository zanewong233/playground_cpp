#ifndef PLAYGROUND_AWAIT_TASK_HPP
#define PLAYGROUND_AWAIT_TASK_HPP
#include <coroutine>
#include <iostream>
#include <thread>

namespace await_task {
struct Task {
  struct promise_type {
    Task get_return_object() {
      return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::suspend_always initial_suspend() noexcept {
      std::cout << __func__ << std::endl;
      return {};
    }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_void() noexcept {}
    void unhandled_exception() { std::terminate(); }
  };

  using handle = std::coroutine_handle<promise_type>;
  handle h_;
  explicit Task(handle h) : h_(h) {}
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
}  // namespace await_task
#endif  // !PLAYGROUND_AWAIT_TASK_HPP
