#pragma once
#include "Engine/Utilities/LRUCache.h"

namespace Engine::Threads
{
  template<Hashable K, typename V>
  class LRUCache
  {
  public:
    using iterator = Engine::LRUCache<K, V>::iterator;

    LRUCache(int size)
      : m_Cache(size) {}

    template<DecaysTo<V> T>
    void insert(const K& key, T&& value)
    {
      std::lock_guard lock(m_Mutex);
      m_Cache[key] = std::forward<T>(value);
    }

    std::optional<V> get(const K& key)
    {
      std::lock_guard lock(m_Mutex);
      iterator cachePosition = m_Cache.find(key);
      return cachePosition == m_Cache.end() ? std::nullopt : std::make_optional<V>(cachePosition->second);
    }

  private:
    std::mutex m_Mutex;
    Engine::LRUCache<K, V> m_Cache;
  };
}