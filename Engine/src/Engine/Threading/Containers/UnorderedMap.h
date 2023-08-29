#pragma once

namespace Engine::Threads
{
  template<Hashable K, Movable V>
    requires std::move_constructible<K>
  class UnorderedMap
  {
  private:
    using iterator = std::unordered_map<K, V>::iterator;
    using const_iterator = std::unordered_map<K, V>::const_iterator;

  public:
    UnorderedMap() = default;

    template<DecaysTo<V> T>
    bool insert(const K& key, T&& value)
    {
      std::lock_guard lock(m_Mutex);
      auto [insertionPosition, insertionSuccess] = m_Data.insert({ key, std::forward<T>(value) });
      return insertionSuccess;
    }

    void erase(const K& key)
    {
      std::lock_guard lock(m_Mutex);
      m_Data.erase(key);
    }

    std::optional<std::pair<K, V>> tryRemoveAny()
    {
      std::lock_guard lock(m_Mutex);

      if (!m_Data.empty())
      {
        std::pair<K, V> keyVal = std::move(*m_Data.begin());
        m_Data.erase(m_Data.begin());
        return keyVal;
      }
      return std::nullopt;
    }

    std::optional<V> get(const K& key) const
    {
      std::lock_guard lock(m_Mutex);
      const_iterator mapPosition = m_Data.find(key);
      return mapPosition == m_Data.end() ? std::nullopt : std::make_optional<V>(mapPosition->second);
    }

    std::unordered_map<K, V> getCurrentState() const
    {
      std::lock_guard lock(m_Mutex);
      return m_Data;
    }

    std::unordered_map<K, V> getSubsetOfCurrentState(const std::vector<K>& keys) const
    {
      std::lock_guard lock(m_Mutex);

      std::unordered_map<K, V> subset;
      for (const K& key : keys)
      {
        const_iterator keyValuePosition = m_Data.find(key);
        if (keyValuePosition != m_Data.end())
          subset.insert(*keyValuePosition);
      }
      return subset;
    }

    bool contains(const K& key) const
    {
      std::lock_guard lock(m_Mutex);
      return m_Data.contains(key);
    }

    bool empty() const
    {
      std::lock_guard lock(m_Mutex);
      return m_Data.empty();
    }

    size_t size() const
    {
      std::lock_guard lock(m_Mutex);
      return m_Data.size();
    }

  private:
    mutable std::mutex m_Mutex;
    std::unordered_map<K, V> m_Data;
  };
}