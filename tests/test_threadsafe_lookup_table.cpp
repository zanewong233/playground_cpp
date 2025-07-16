#include <gtest/gtest.h>

#include "playground/threading/threadsafe_lookup_table.hpp"

using namespace playground;
TEST(ThreadsafeLookupTableTest, AddGetRemove) {
  ThreadsafeLookupTable<std::string, std::string> tb;

  std::string key("name"), value;
  value = tb.ValueFor(key, "null");
  ASSERT_EQ(value, "null");

  tb.AddOrUpdate("name", "jack");
  value = tb.ValueFor(key, "null");
  ASSERT_EQ(value, "jack");

  tb.AddOrUpdate("age", "18");
  value = tb.ValueFor("age", "null");
  ASSERT_EQ(value, "18");

  tb.Remove(key);
  value = tb.ValueFor(key, "null");
  ASSERT_EQ(value, "null");
}

TEST(ThreadsafeLookupTableTest, GetMap) {
  ThreadsafeLookupTable<std::string, std::string> tb;
  tb.AddOrUpdate("name", "jack");
  tb.AddOrUpdate("age", "18");
  std::map<std::string, std::string> except_map{{"name", "jack"}, {"age", "18"}};
  ASSERT_EQ(tb.GetMap(), except_map);
}
