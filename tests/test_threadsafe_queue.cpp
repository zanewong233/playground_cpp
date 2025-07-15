#include <gtest/gtest.h>

#include "playground/threading/threadsafe_queue.hpp"

using namespace playground;

TEST(ThreadsafeQueueTest, PushPop) {
  ThreadsafeQueue<int> que;
  auto res = que.try_pop();
  EXPECT_FALSE(res);

  que.push(1);
  res = que.try_pop();
  EXPECT_EQ(*res, 1);

  que.push(2);
  que.push(3);
  que.push(4);
  que.push(5);
  res = que.try_pop();
  EXPECT_EQ(*res, 2);
  res = que.try_pop();
  EXPECT_EQ(*res, 3);
  res = que.try_pop();
  EXPECT_EQ(*res, 4);
  res = que.try_pop();
  EXPECT_EQ(*res, 5);
  res = que.try_pop();
  EXPECT_FALSE(res);
}
