#pragma once
#include "Engine/Utilities/LRUCache.h"

namespace Engine::Threads
{
  template<Hashable K, typename V>
  class LRUCache : private NonCopyable, NonMovable
  {
    using const_iterator = Engine::LRUCache<K, std::shared_ptr<V>>::const_iterator;

  public:

    LRUCache(int size)
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
      const_iterator cachePosition = m_Cache.find(key);
      return cachePosition == m_Cache.end() ? nullptr : cachePosition->second;
    }

  private:
    std::mutex m_Mutex;
    Engine::LRUCache<K, std::shared_ptr<V>> m_Cache;
  };
}