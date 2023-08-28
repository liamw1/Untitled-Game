#pragma once

namespace Engine::Threads
{
  template<Hashable K, Movable V>
    requires std::move_constructible<K>
  class UnorderedMapQueue
  {
  public:
    UnorderedMapQueue() = default;

    template<DecaysTo<V> T>
    void insert(const K& key, T&& value)
    {
      std::lock_guard lock(m_Mutex);
      m_Data[key] = std::forward<T>(value);
    }

    std::optional<std::pair<K, V>> tryRemove()
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

    size_t empty()
    {
      std::lock_guard lock(m_Mutex);
      return m_Data.empty();
    }

  private:
    std::mutex m_Mutex;
    std::unordered_map<K, V> m_Data;
  };
}