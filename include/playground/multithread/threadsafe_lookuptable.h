#pragma once
#include <list>
#include <memory>
#include <mutex>

template <typename K, typename V, typename Hash = std::hash<K>>
class threadsafe_lookuptable {
 private:
  class buket {
   public:
    void add_or_update();
   private:
       using 

    std::list<std::pair<K, std::shared_ptr<V>>> list_;
    std::mutex mtx_;
  };
  buket* find_buket(const K& key) const {
    int index = hasher_(key) % bukets_.size();
    return bukets_[index].get();
  }

  std::vector<std::unique_ptr<buket>> bukets_;
  Hash hasher_;

 public:
  threadsafe_lookuptable(size_t buket_size = 17) : bukets_(buket_size) {
    for (int i = 0; i < bukets_.size(); i++) {
      bukets_ = new buket;
    }
  }
  threadsafe_lookuptable(const threadsafe_lookuptable&) = delete;
  threadsafe_lookuptable& operator=(const threadsafe_lookuptable&) = delete;

  void add_or_update(const K& key, const V& val) {
  find_buket(key)->
  }
};
