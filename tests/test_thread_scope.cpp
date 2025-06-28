#include <gtest/gtest.h>

#include <chrono>

#include "playground/threading/scoped_thread.hpp"

using namespace playground;
using namespace std::chrono_literals;

// 构造函数在接收无效线程时会抛出异常
TEST(ScopedThreadTest, ConstructorThrowsOnNonJoinableThread) {
  std::thread invalidThread;  // 默认构造的线程是不可连接的

  ASSERT_THROW(
      { ScopedThread scopeThread(std::move(invalidThread)); },
      std::logic_error);
}

// ------------------------------------------------------------------
// 测试点2：析构函数在对象离开作用域时，必须 join 线程
// ------------------------------------------------------------------
TEST(ScopedThreadTest, DestructorJoinsThreadOnScopeExit) {
  // 1. Arrange: 创建一个线程安全的状态标志
  // 使用 std::atomic<bool> 确保在多线程中读写布尔值是安全的，不会产生数据竞争
  std::atomic<bool> thread_ran = false;

  // 创建一个作用域块
  {
    // 2. Act:
    // 创建一个 lambda 函数作为线程的执行体
    auto thread_func = [&thread_ran]() {
      // 模拟一些工作
      std::this_thread::sleep_for(10ms);
      // 设置标志为 true，表示线程已经执行到这里
      thread_ran = true;
    };

    // 创建一个 std::thread 对象，并立即用它构造 scoped_thread
    // st 的生命周期仅限于这个花括号 {} 内
    ScopedThread st(std::thread(std::move(thread_func)));

  }  // <-- 当代码执行到这里时，st 对象被销毁，其析构函数 ~scoped_thread()
     // 被调用

  // 3. Assert:
  // 因为析构函数调用了 t.join()，程序会在这里等待线程执行完毕。
  // 所以，当代码继续执行到这里时，thread_ran 的值必然已经被线程修改为 true。
  // 如果没有 join，主线程可能先于子线程结束，这个断言就会失败。
  ASSERT_TRUE(thread_ran);
}

// ------------------------------------------------------------------
// 测试点3：测试构造函数在接收有效线程时，不会抛出异常
// ------------------------------------------------------------------
TEST(ScopedThreadTest, ConstructorSucceedsOnJoinableThread) {
  // 1. Arrange: 创建一个有效的线程
  std::thread valid_thread([] { /* do nothing */ });

  // 2. Act & Assert:
  // 使用 GTest 的 ASSERT_NO_THROW 宏来断言构造过程不抛出任何异常
  ASSERT_NO_THROW(ScopedThread st(std::move(valid_thread)));
}