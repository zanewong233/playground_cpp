#include "playground/cpp17/cpp17_test.h"

#include <iostream>
#include <iterator>
#include <string>

#include "playground/cpp17/cpp17_test_other.h"

namespace playground {
namespace old {
template <typename T>
T sum_all(T val) {
  return val;
}

template <typename T, typename... Args>
T sum_all(T first, Args... args) {
  return first + sum_all(args...);
}
}  // namespace old

namespace modern {
template <typename... Args>
auto sum_all(Args... args) {
  return (args + ... + 0);
}
}  // namespace modern

//=================================

template <typename T, typename = void>
struct has_type_member : std::false_type {};

template <typename T>
struct has_type_member<T, std::void_t<typename T::type>> : std::true_type {};

template <typename T>
inline constexpr bool has_type_member_v = has_type_member<T>::value;

struct Foo {
  using type = int;
};
struct Bar {};

//=================================
class MyObject {
 public:
  MyObject(std::string_view name, int val) : name_(name), val_(val) {
    std::cout << "MyObject: '" << name_ << "' constructed." << std::endl;
  }
  ~MyObject() {
    std::cout << "MyObject: '" << name_ << "' destructed." << std::endl;
  }
  void print() const {
    std::cout << "name: " << name_ << ", value: " << val_ << std::endl;
  }

 private:
  std::string name_;
  int val_;
};

template <typename T>
class MyOptional {
 public:
  MyOptional() = default;
  ~MyOptional() {
    if (has_value_) {
      destory();
    }
  }

  void destory() {
    if (has_value_) {
      get()->~T();
      has_value_ = false;
    }
  }

  template <typename... Args>
  void emplace(Args&&... args) {
    if (has_value_) {
      destory();
    }
    ::new (&buffer_) T(std::forward<Args>(args)...);
    has_value_ = true;
  }

  T* get() { return reinterpret_cast<T*>(&buffer_); }
  const T* get() const { return reinterpret_cast<const T*>(&buffer_); }

  T& operator*() { return *get(); }
  const T& operator*() const { return *get(); }
  T* operator->() { return get(); }
  const T* operator->() const { return get(); }

 private:
  // std::aligned_storage_t<sizeof(T), alignof(T)> buffer_;
  alignas(T) std::byte buffer_[sizeof(T)];
  bool has_value_ = false;
};

class SmallObj {
 public:
  SmallObj(int16_t v) : val_(v) {}
  SmallObj(const SmallObj& rhs) { val_ = rhs.val_; }
  void print() { std::cout << "value is:" << val_ << std::endl; }

 private:
  int16_t val_;
};

void cpp17_test::test() {
  constexpr int sz = sizeof(SmallObj);
  auto stor = std::aligned_storage_t<sz, alignof(SmallObj)>();

  SmallObj obj(233);
  ::new (&stor) SmallObj(obj);

  auto pStor = &stor;
  SmallObj** ppObj = reinterpret_cast<SmallObj**>(&stor);
  SmallObj* pObj = *ppObj;
  pObj->print();

  int a = 10;
  a++;

  // std::shared_ptr<MyOptional<MyObject>> opt =
  // std::make_shared<MyOptional<MyObject>>();
  // opt->emplace("john", 12);
  //(*opt)->print();
  // opt->emplace("jack", 22);

  // bool res = std::is_nothrow_move_constructible_v<MyObject>;

  // Test variadic template function
  // int result = modern::sum_all(1, 2, 3, 4, 5);
  // std::cout << "Sum of all arguments: " << result << std::endl;

  // std::cout << std::boolalpha;
  // std::cout << "Foo has ::type?" << has_type_member_v<Foo> << std::endl;
  // std::cout << "Bar has ::type?" << has_type_member_v<Bar> << std::endl;
}
}  // namespace playground
