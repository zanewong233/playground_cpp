#include "concurrent/for_each.hpp"

using namespace playground::parallel;

void func() {
  int a = 10;
  a++;
}

void func1() {
  int a = 10;
  a++;
}

int main() {
  func();
  func1();

  int a = 10;
  a++;

  return 0;
}
