#include <coroutine>
#include <iostream>
#include <optional>

// Generator 不是必须的，是一种约定俗成的较好的实现方式。
// 主要用来封装handle 的行为。
struct Generator {
  struct promise_type {
    std::optional<int> current_value_;
    Generator get_return_object() {
      return Generator{
          std::coroutine_handle<promise_type>::from_promise(*this)};
    }

    std::suspend_always initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    // 把值保存到promise ，然后挂起协程，等待外部resume 时再从下一行继续
    std::suspend_always yield_value(int value) {
      current_value_ = value;
      return {};
    }
    void return_void() {}
    void unhandled_exception() { std::exit(1); }
  };

  // handle 对象是协程运行的遥控器
  std::coroutine_handle<promise_type> handle_;

  explicit Generator(std::coroutine_handle<promise_type> h) : handle_(h) {}

  ~Generator() {
    if (handle_) {
      handle_.destroy();
    }
  }

  bool next() {
    // 如果协程执行到了final suspend ，并且不会被恢复，则返回true
    if (!handle_.done()) {
      handle_.resume();
    }
    return !handle_.done();
  }

  int value() { return *handle_.promise().current_value_; }
};

// 调用函数创建协程实例
// 创建函数的要素：
//  1. 至少有一个co_wait/co_yeild/co_return
//  2. 返回类型：能提供promise_type
//  3. promise_type 要符合规定
// 其行为可以用以下伪代码理解：
/*auto g = [&]() {
  // 分配帧
  frame = operator new(sizeof(Frame));
  // 构造 promise
  new (&frame->promise) promise_type{};
  // 拷贝实参 n=5
  frame->n = 5;
  // 返回外壳对象（里头带 handle ）
  Generator ret = frame->promise.get_return_object();
  // 初始挂起
  if (frame->promise.initial_suspend().await_ready() == false) {
    // 标记为“挂起状态”
  }
  return ret;  // 给 g
}();
*/
Generator counter(int n) {
  for (int i = 0; i < n; i++) {
    // 编译后翻译为：promise_type.yield_value(i)
    // yield_value() 函数的返回值决定调用后是否挂起
    co_yield i;
  }

  // 调用promise 的return_void()
  // 如果没写，编译器会自动加上，和return 关键字差不多
  co_return;
}

int main() {
  auto g = counter(5);
  while (g.next()) {
    std::cout << g.value() << " ";
  }

  return 0;
}
