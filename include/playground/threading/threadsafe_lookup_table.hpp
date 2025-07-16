#ifndef PLAYGROUND_THREADING_THREADSAFE_LOOKUP_TABLE_H_
#define PLAYGROUND_THREADING_THREADSAFE_LOOKUP_TABLE_H_
#include <map>
#include <memory>
#include <shared_mutex>
#include <vector>

namespace playground {
template <typename Key, typename Value, typename Hash = std::hash<Key>>
class ThreadsafeLookupTable {
 public:
  explicit ThreadsafeLookupTable(int bucket_num = 19,
                                 const Hash& hasher = Hash{})
      : buckets_(bucket_num), hasher_(hasher) {
    for (int i = 0; i < bucket_num; i++) {
      buckets_[i] = std::make_unique<BucketType>();
    }
  }
  ThreadsafeLookupTable(const ThreadsafeLookupTable&) = delete;
  ThreadsafeLookupTable& operator=(ThreadsafeLookupTable&) = delete;

  Value ValueFor(const Key& key, const Value& default_value) {
    return FindBucket(key)->ValueFor(key, default_value);
  }

  void AddOrUpdate(const Key& key, const Value& val) {
    return FindBucket(key)->AddOrUpdate(key, val);
  }

  void Remove(const Key& key) { return FindBucket(key)->Remove(key); }

  std::map<Key, Value> GetMap() const {
    std::vector<std::unique_lock<std::shared_mutex>> locks;
    for (auto& b : buckets_) {
      locks.push_back(std::unique_lock(b->mtx_));
    }

    std::map<Key, Value> res;
    for (auto& b : buckets_) {
      for (const auto& it : b->data_) {
        res[it.first] = it.second;
      }
    }
    return res;
  }

 private:
  class BucketType {
   public:
    Value ValueFor(const Key& key, const Value& default_value) {
      std::shared_lock lock(mtx_);
      auto it = FindEntryFor(key);
      return it == data_.end() ? default_value : it->second;
    }

    void AddOrUpdate(const Key& key, const Value& val) {
      std::lock_guard lock(mtx_);
      auto it = FindEntryFor(key);
      if (it == data_.end()) {
        data_.push_back({key, val});
      } else {
        it->second = val;
      }
    }

    void Remove(const Key& key) {
      std::lock_guard lock(mtx_);
      auto it = FindEntryFor(key);
      if (it != data_.end()) {
        data_.erase(it);
      }
    }

   private:
    using BucketValue = std::pair<Key, Value>;
    using BucketData = std::list<BucketValue>;
    using BucketIterator = typename BucketData::iterator;

    BucketIterator FindEntryFor(const Key& key) {
      return std::find_if(
          data_.begin(), data_.end(),
          [&key](const BucketValue& item) { return item.first == key; });
    }

   private:
    friend class ThreadsafeLookupTable;

    BucketData data_;
    mutable std::shared_mutex mtx_;
  };

  BucketType* FindBucket(const Key& key) const {
    auto index = hasher_(key) % buckets_.size();
    return buckets_[index].get();
  }

 private:
  std::vector<std::unique_ptr<BucketType>> buckets_;
  std::hash<Key> hasher_;
};
}  // namespace playground
#endif
