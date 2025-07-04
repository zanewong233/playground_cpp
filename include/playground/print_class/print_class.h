#pragma once
#include <iostream>

namespace playground {
class MessagePrinter {
 public:
  MessagePrinter(unsigned long val = 0ul) : data_(val) {
    std::cout << "Constructor called" << std::endl;
  }
  MessagePrinter(const MessagePrinter& other) : data_(other.data_) {
    std::cout << "Copy constructor called" << std::endl;
  }
  MessagePrinter(MessagePrinter&& other) noexcept
      : data_(std::move(other.data_)) {
    std::cout << "Move constructor called" << std::endl;
  }
  MessagePrinter& operator=(const MessagePrinter& other) {
    std::cout << "Copy assignment operator called" << std::endl;
    if (this == &other) {
      return *this;
    }
    data_ = other.data_;
    return *this;
  }
  MessagePrinter& operator=(MessagePrinter&& other) noexcept {
    std::cout << "Move assignment operator called" << std::endl;
    if (this == &other) {
      return *this;
    }
    data_ = std::move(other.data_);
    return *this;
  }
  ~MessagePrinter() noexcept { std::cout << "Destructor called" << std::endl; }

  std::string GetMessage(const std::string& name) const;

 private:
  unsigned long data_ = 0;
};
}  // namespace playground
