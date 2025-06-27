#include "playground/print_class/print_class.h"

namespace playground {
std::string MessagePrinter::GetMessage(const std::string& name) const {
  if (name.empty()) {
    return "Hello, stranger!";
  }
  return "Hello, " + name + "!";
}
}  // namespace playground
