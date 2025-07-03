#pragma once
#include <iostream>

namespace playground {
class MessagePrinter {
 public:
  MessagePrinter() { std::cout << "Constructor called" << std::endl; }
  MessagePrinter(const MessagePrinter& other) {
    std::cout << "Copy constructor called" << std::endl;
  }
  MessagePrinter(MessagePrinter&& other) noexcept {
    std::cout << "Move constructor called" << std::endl;
  }
  MessagePrinter& operator=(const MessagePrinter& other) {
    std::cout << "Copy assignment operator called" << std::endl;
    return *this;
  }
  MessagePrinter& operator=(MessagePrinter&& other) noexcept {
    std::cout << "Move assignment operator called" << std::endl;
    return *this;
  }
  ~MessagePrinter() noexcept { std::cout << "Destructor called" << std::endl; }

  std::string GetMessage(const std::string& name) const;
};
}  // namespace playground
