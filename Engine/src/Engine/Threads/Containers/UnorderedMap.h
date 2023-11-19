#pragma once
#include "Engine/Core/Concepts.h"

namespace eng::thread
{
  template<Hashable K, typename V>
    requires std::is_default_constructible_v<K> && std::move_constructible<K>
  class UnorderedMap : private SetInStone
  {
    mutable std::shared_mutex m_Mutex;
    std::unordered_map<K, std::shared_ptr<V>> m_Data;

  public:
    UnorderedMap() = default;

    bool insert(const K& key, const std::shared_ptr<V>& valuePointer)
    {
      std::lock_guard lock(m_Mutex);
      auto [insertionPosition, insertionSuccess] = m_Data.emplace(key, valuePointer);
      return insertionSuccess;
    }

    template<DecaysTo<V> T>
    bool insert(const K& key, T&& value)
    {
      std::lock_guard lock(m_Mutex);
      auto [insertionPosition, insertionSuccess] = m_Data.emplace(key, std::make_shared<V>(std::forward<T>(value)));
      return insertionSuccess;
    }

    bool erase(const K& key)
    {
      std::lock_guard lock(m_Mutex);
      uSize elementsErased = m_Data.erase(key);
      return elementsErased > 0;
    }

    std::pair<K, std::shared_ptr<V>> tryRemoveAny()
    {
      std::pair<K, std::shared_ptr<V>> keyValue{};

      std::lock_guard lock(m_Mutex);
      if (!m_Data.empty())
      {
        keyValue = std::move(*m_Data.begin());
        m_Data.erase(m_Data.begin());
      }
      return keyValue;
    }

    std::shared_ptr<V> get(const K& key) const
    {
      std::shared_lock lock(m_Mutex);
      auto mapPosition = m_Data.find(key);
      std::shared_ptr<V> copy = mapPosition == m_Data.end() ? nullptr : mapPosition->second;
      return copy;
    }

    template<std::predicate<const K&> F>
    std::vector<K> getKeys(const F& condition) const
    {
      std::vector<K> keysMatchingCondition;

      std::shared_lock lock(m_Mutex);
      for (const auto& [key, value] : m_Data)
        if (condition(key))
          keysMatchingCondition.push_back(key);
      return keysMatchingCondition;
    }

    std::unordered_map<K, std::shared_ptr<V>> getCurrentState() const
    {
      std::shared_lock lock(m_Mutex);
      return m_Data;
    }

    bool contains(const K& key) const
    {
      std::shared_lock lock(m_Mutex);
      return m_Data.contains(key);
    }

    bool empty() const
    {
      std::shared_lock lock(m_Mutex);
      return m_Data.empty();
    }

    uSize size() const
    {
      std::shared_lock lock(m_Mutex);
      return m_Data.size();
    }
  };
}