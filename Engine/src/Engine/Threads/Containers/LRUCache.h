#pragma once
#include "Engine/Utilities/LRUCache.h"

namespace eng::threads
{
  template<Hashable K, typename V>
  class LRUCache : private NonCopyable, NonMovable
  {
    std::mutex m_Mutex;
    eng::LRUCache<K, std::shared_ptr<V>> m_Cache;

  public:
    LRUCache(i32 size)
      : m_Cache(size) {}

    bool insert(const K& key, const std::shared_ptr<V>& valuePointer)
    {
      std::lock_guard lock(m_Mutex);
      auto [insertionPosition, insertionSuccess] = m_Cache.insert(key, valuePointer);
      return insertionSuccess;
    }

    template<DecaysTo<V> T>
    bool insert(const K& key, T&& value)
    {
      std::lock_guard lock(m_Mutex);
      std::shared_ptr valuePointer = std::make_shared<V>(std::forward<T>(value));
      auto [insertionPosition, insertionSuccess] = m_Cache.insert(key, valuePointer);
      return insertionSuccess;
    }

    std::shared_ptr<V> get(const K& key)
    {
      std::lock_guard lock(m_Mutex);
      auto cachePosition = m_Cache.find(key);
      return cachePosition == m_Cache.end() ? nullptr : cachePosition->second;
    }
  };
}