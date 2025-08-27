#include <coroutine>
#include <iostream>

struct SimpleAwaiter {
  int* p_ = nullptr;
  bool await_ready() const noexcept { return *p_ == 0; }  // 0 就绪，不会挂起
  void await_suspend(std::coroutine_handle<>) {}
  int await_resume() const noexcept { return (*p_)--; }
};

struct Task {
  struct promise_type {
    Task get_return_object() {
      return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::suspend_always initial_suspend() noexcept { return {}; }
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
    }
  }
};

Task demo_co_await(int n) {
  int counter = n;
  while (true) {
    // 编译后的co_await 模拟展开
    // auto&& __expr = SimpleAwaiter{&counter};  // 原始表达式
    // auto __awaiter =
    //     get_awaiter(__expr);  // 提取 awaiter（可能是 operator co_await
    //     或成员）
    // if (!__awaiter.await_ready()) {
    //   // 保存协程当前状态（局部变量 counter 的值，程序计数点等）到协程帧
    //   __awaiter.await_suspend(this_coroutine_handle);
    //   co_suspend;  // 真正挂起协程，返回到调用方
    // }
    //// 协程恢复时，从这里继续
    // int v = __awaiter.await_resume();

    int v = co_await SimpleAwaiter{&counter};
    if (v == 0) {
      break;
    }
    std::cout << "got " << v << "\n";
  }
}

int main() {
  auto g = demo_co_await(3);
  g.run_to_end();
  return 0;
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
