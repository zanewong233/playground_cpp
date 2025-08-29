// 符合co_await 表达式情况1：表达式直接是一个waiter 对象
#ifndef PLAYGROUND_AWAIT_1_HPP
#define PLAYGROUND_AWAIT_1_HPP
#include <coroutine>
#include <iostream>
#include <thread>

#include "common.hpp"

namespace await_1 {
struct SimpleAwaiter {
  int* p_ = nullptr;
  bool await_ready() const noexcept {
    LOG_FUNC();
    return *p_ == 0;
  }  // 0 就绪，不会挂起
  void await_suspend(std::coroutine_handle<>) { LOG_FUNC(); }
  int await_resume() const noexcept {
    LOG_FUNC();
    return (*p_)--;
  }
};

::common::Task foo(int n) {
  int counter = n;
  while (true) {
    int v = co_await SimpleAwaiter{&counter};
    if (v == 0) {
      break;
    }
    std::cout << "got " << v << "\n";
  }
}

void run_example() {
  auto g = foo(3);
  g.run_to_end();
}

// =================更真实的展开=============
// struct __demo_frame {
//  Task::promise_type promise;
//  int counter;
//  int v;
//  enum { start, after_await, done } state;
//};
//
// Task demo_co_await(int n) {
//  // 构造帧 + promise
//  __demo_frame* frame = new __demo_frame;
//  frame->promise = {};
//  frame->counter = n;
//  frame->state = start;
//
//  auto h =
//      std::coroutine_handle<Task::promise_type>::from_promise(frame->promise);
//  Task ret = frame->promise.get_return_object();
//
//  if (!frame->promise.initial_suspend().await_ready()) {
//    frame->promise.initial_suspend().await_suspend(h);
//    return ret;
//  }
//
// resume_entry:
//  switch (frame->state) {
//    case start: {
//      while (true) {
//        auto __awaiter = SimpleAwaiter{&frame->counter};
//        if (!__awaiter.await_ready()) {
//          frame->state = after_await;
//          if (__awaiter.await_suspend(h)) return ret;  // 挂起
//        }
//        frame->v = __awaiter.await_resume();
//        if (frame->v == 0) goto done_state;
//        std::cout << "got " << frame->v << "\n";
//      }
//    }
//    case after_await: {
//      // 恢复后继续执行 await_resume
//      frame->v = /* 从保存的 awaiter */.await_resume();
//      if (frame->v == 0) goto done_state;
//      std::cout << "got " << frame->v << "\n";
//      frame->state = start;
//      goto resume_entry;  // 回到循环
//    }
//    default:
//      break;
//  }
// done_state:
//  frame->promise.return_void();
//  frame->promise.final_suspend().await_suspend(h);
//  return ret;
//}
}  // namespace await_1
#endif  // !PLAYGROUND_AWAIT_1_HPP
