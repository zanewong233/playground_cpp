#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

#include "playground/threading/joining_thread.hpp"

using namespace playground;

TEST(JoiningThreadTest, DefaultConstructorNotJoinable) {
  JoiningThread jt;
  EXPECT_FALSE(jt.Joinable());
}

TEST(JoiningThreadTest, LaunchWithCallableAndJoin) {
  std::atomic<bool> flag{false};
  JoiningThread jt([&flag]() { flag = true; });
  EXPECT_TRUE(jt.Joinable());
  jt.Join();
  EXPECT_FALSE(jt.Joinable());
  EXPECT_TRUE(flag);
}

TEST(JoiningThreadTest, LaunchWithCallableAndDetach) {
  std::atomic<bool> flag{false};
  {
    JoiningThread jt([&flag]() { flag = true; });
    EXPECT_TRUE(jt.Joinable());
    jt.Detach();
    EXPECT_FALSE(jt.Joinable());
  }
  // 等待线程执行完毕
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_TRUE(flag);
}

TEST(JoiningThreadTest, MoveConstructor) {
  std::atomic<bool> flag{false};
  JoiningThread jt1([&flag]() { flag = true; });
  EXPECT_TRUE(jt1.Joinable());
  JoiningThread jt2(std::move(jt1));
  EXPECT_FALSE(jt1.Joinable());
  EXPECT_TRUE(jt2.Joinable());
  jt2.Join();
  EXPECT_TRUE(flag);
}

TEST(JoiningThreadTest, MoveAssignmentFromJoiningThread) {
  std::atomic<int> counter{0};
  JoiningThread jt1([&counter]() { ++counter; });
  EXPECT_TRUE(jt1.Joinable());
  JoiningThread jt2;
  jt2 = std::move(jt1);
  EXPECT_FALSE(jt1.Joinable());
  EXPECT_TRUE(jt2.Joinable());
  jt2.Join();
  EXPECT_EQ(counter.load(), 1);
}

TEST(JoiningThreadTest, AssignmentFromStdThread) {
  std::atomic<int> counter{0};
  std::thread th([&counter]() { ++counter; });
  EXPECT_TRUE(th.joinable());

  JoiningThread jt;
  jt = std::move(th);
  EXPECT_FALSE(th.joinable());
  EXPECT_TRUE(jt.Joinable());
  jt.Join();
  EXPECT_EQ(counter.load(), 1);
}

TEST(JoiningThreadTest, SwapThreads) {
  std::atomic<int> a{0}, b{0};
  JoiningThread jt1([&a]() {
    ++a;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  });
  JoiningThread jt2([&b]() {
    ++b;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  });

  auto id1 = jt1.GetId();
  auto id2 = jt2.GetId();
  EXPECT_NE(id1, id2);

  jt1.Swap(jt2);
  EXPECT_EQ(jt1.GetId(), id2);
  EXPECT_EQ(jt2.GetId(), id1);

  jt1.Join();
  jt2.Join();
  EXPECT_EQ(a.load(), 1);
  EXPECT_EQ(b.load(), 1);
}

TEST(JoiningThreadTest, DestructorJoinsAutomatically) {
  std::atomic<bool> done{false};
  {
    JoiningThread jt([&done]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      done = true;
    });
    EXPECT_TRUE(jt.Joinable());
  }
  // 离开作用域后，析构应当已经 join，thread 完成
  EXPECT_TRUE(done);
}
