#pragma once
#include <type_traits>

namespace cpp17_test {
template <typename T, bool EnableSSO = true>
class pimpl {
 public:
  template <typename... Args>
  pimpl(Args... args) {
    if constexpr (use_sso_v) {
      ::new (&storage_) T(std::forward<Args>(args)...);
    } else {
      T* ptr = ::new T(std::forward<Args>(args)...);
      ::new (&storage_) T*(ptr);
    }
  }
  ~pimpl() {
    if constexpr (use_sso_v) {
      get()->~T();
    } else {
      delete get_heap_ptr();
    }
  }

  pimpl(const pimpl&) = delete;
  pimpl& operator=(const pimpl&) = delete;

  pimpl(pimpl&& rhs) noexcept {
    if constexpr (use_sso_v) {
      ::new (&storage_) T(std::move(*rhs.get()));
    } else {
      ::new (&storage_) T*(rhs.get_heap_ptr());
      ::new (&rhs.storage_) T*(nullptr);
    }
  }
  pimpl& operator=(pimpl&& rhs) {
    if (this != &rhs) {
      this->~pimpl();
      ::new (this) pimpl(std::move(rhs));
    }
    return *this;
  }

  T* operator->() noexcept { return get(); }
  const T* operator->() const noexcept { return get(); }

  T& operator*() noexcept { return *get(); }
  const T& operator*() const noexcept { return *get(); }

 private:
  T*& get_heap_ptr() noexcept { return *reinterpret_cast<T**>(&storage_); }
  const T*& get_heap_ptr() const noexcept {
    return *reinterpret_cast<const T* const*>(&storage_);
  }

  T* get() noexcept {
    if constexpr (use_sso_v) {
      return std::launder(reinterpret_cast<T*>(&storage_));
    } else {
      return get_heap_ptr();
    }
  }
  const T* get() const noexcept {
    if constexpr (use_sso_v) {
      return std::launder(reinterpret_cast<const T*>(&storage_));
    } else {
      return get_heap_ptr();
    }
  }

 private:
  static constexpr bool use_sso_v = EnableSSO && (sizeof(T) <= sizeof(void*)) &&
                                    std::is_nothrow_move_constructible_v<T>();

  using storage_t = std::aligned_storage_t<
      (sizeof(T) > sizeof(void*) ? sizeof(T) : sizeof(void*)), alignof(T)>;
  storage_t storage_;
};

}  // namespace cpp17_test