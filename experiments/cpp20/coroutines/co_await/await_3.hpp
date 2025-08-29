// 符合co_await 表达式情况3：表达式可以通过await_transform 函数转为awaiter 对象
#ifndef PLAYGROUND_AWAIT_3_HPP
#define PLAYGROUND_AWAIT_3_HPP
#include <coroutine>
#include <iostream>
#include <thread>

#include "common.hpp"

namespace await_3 {
struct MyPromise {
  auto get_return_object() { /*...*/ }
  std::suspend_always initial_suspend() noexcept { return {}; }
  std::suspend_always final_suspend() noexcept { return {}; }
  void return_void() {}
  void unhandled_exception() {}

  // 定义 await_transform，拦截 co_await
  template <typename T>
  auto await_transform(T&& value) {
    // 强制把任何 int 类型包装成一个 awaiter
    struct IntAwaiter {
      int v;
      bool await_ready() noexcept { return true; }
      void await_suspend(std::coroutine_handle<>) noexcept {}
      int await_resume() noexcept { return v * 10; }
    };
    if constexpr (std::is_same_v<std::decay_t<T>, int>)
      return IntAwaiter{(int)value};
    else
      return std::forward<T>(value);
  }
};

::await_task::Task foo() {
  int v = co_await 5;
  co_return v;
}

void run_example() {
  auto g = foo();
  g.run_to_end();
}
}  // namespace await_3
#endif
