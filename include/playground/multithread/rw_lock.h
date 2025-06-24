#pragma once
#include <memory>
#include <mutex>

class rw_lock {
 private:
  std::mutex mtx_;
  std::condition_variable rd_cond_;
  std::condition_variable wt_cond_;
  bool writing_ = false;
  unsigned int readers_ = 0;
  unsigned int writer_waiting_ = 0;
  static inline thread_local std::map<void*, unsigned int> lock_cnt_ = {};

 public:
  void lock_read() {
    std::unique_lock<std::mutex> lock(mtx_);
    if (lock_cnt_[this] > 0) {
      ++(lock_cnt_[this]);
    } else {
      rd_cond_.wait(lock, [&]() { return !writing_ && writer_waiting_ == 0; });
    }
    ++readers_;
  }

  void unlock_read() {
    std::unique_lock<std::mutex> lock(mtx_);
    if (--(lock_cnt_[this]) == 0 && --readers_ == 0 && writer_waiting_ > 0) {
      wt_cond_.notify_one();
    }
  }

  void lock_write() {
    std::unique_lock<std::mutex> lock(mtx_);
    if (lock_cnt_[this] == 0 || !writing_) {
      ++writer_waiting_;
      wt_cond_.wait(lock, [&]() { return !writing_ && readers_ == 0; });
      --writer_waiting_;
    }
    ++(lock_cnt_[this]);
    writing_ = true;
  }

  void unlock_write() {
    std::unique_lock<std::mutex> lock(mtx_);
    if (--(lock_cnt_[this]) == 0) {
      writing_ = false;
      if (writer_waiting_ > 0) {
        wt_cond_.notify_one();
      } else {
        rd_cond_.notify_all();
      }
    }
  }
};
