#ifndef PLAYGROUND_BASE_MACROS_H_
#define PLAYGROUND_BASE_MACROS_H_
#include <iostream>

// 通用函数签名宏
#if defined(_MSC_VER)
#define FUNC_SIG __FUNCSIG__
#elif defined(__GNUC__) || defined(__clang__)
#define FUNC_SIG __PRETTY_FUNCTION__
#else
#define FUNC_SIG __func__
#endif

// 输出当前函数的辅助宏
#define LOG_FUNC()                                   \
  do {                                               \
    std::cout << "[FUNC] " << FUNC_SIG << std::endl; \
  } while (0)

#endif
