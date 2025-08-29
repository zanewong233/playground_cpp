// 符合co_await 表达式情况2：表达式可以通过operator await函数转为awaiter 对象
#ifndef PLAYGROUND_AWAIT_2_HPP
#define PLAYGROUND_AWAIT_2_HPP
#include <coroutine>
#include <iostream>
#include <thread>

#include "common.hpp"

namespace await_2 {
struct MyAwaitable {
  auto operator co_await() const noexcept {
    struct Awaiter {
      bool await_ready() noexcept { return true; }
      bool await_suspend(std::coroutine_handle<>) noexcept {}
      int await_resume() noexcept { return 42; }
    };
    return Awaiter{};
  }
};

::await_task::Task foo() {
  int v = co_await MyAwaitable{};
  co_return v;
}

void run_example() {
  auto g = foo();
  g.run_to_end();
}
}  // namespace await_2
#endif  // !PLAYGROUND_AWAIT_2_HPP
