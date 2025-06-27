// 1. 包含 GTest 的主头文件
#include <gtest/gtest.h>

// 2. 包含你需要测试的那个类的头文件
//    根据我们之前的CMake配置，可以直接这样包含
#include "playground/print_class/print_class.h" 

// 使用我们定义的命名空间
using namespace playground;

// 3. 编写第一个测试用例
// 测试套件名为 MessagePrinterTest
// 测试名为 ReturnsCorrectMessageForGivenName
TEST(ThreadScopeTest, NomalTest) {
    // Arrange: 准备测试对象和输入
    MessagePrinter printer;
    std::string name = "World";
    std::string expected_message = "Hello, World!";

    // Act: 执行被测试的方法
    std::string actual_message = printer.GetMessage(name);

    // Assert: 使用断言检查结果是否符合预期
    EXPECT_EQ(actual_message, expected_message);
}