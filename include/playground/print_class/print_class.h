#pragma once
#include <iostream>

class MyClass {
 public:
  MyClass() { std::cout << "Constructor called" << std::endl; }
  MyClass(const MyClass& other) = delete;
  // MyClass(const MyClass& other) {
  //   std::cout << "Copy constructor called" << std::endl;
  // }
  MyClass(MyClass&& other) noexcept {
    std::cout << "Move constructor called" << std::endl;
  }
  MyClass& operator=(const MyClass& other) = delete;
  // MyClass& operator=(const MyClass& other) {
  //   std::cout << "Copy assignment operator called" << std::endl;
  //   return *this;
  // }
  MyClass& operator=(MyClass&& other) noexcept {
    std::cout << "Move assignment operator called" << std::endl;
    return *this;
  }
  ~MyClass() noexcept { std::cout << "Destructor called" << std::endl; }
};

