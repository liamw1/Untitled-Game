#pragma once

namespace Threads
{
  template<typename K, typename V>
  class UnorderedMapQueue
  {
  public:
    UnorderedMapQueue() = default;

    template<typename T>
    void add(const K& key, T&& value)
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

    std::size_t empty()
    {
      std::lock_guard lock(m_Mutex);
      return m_Data.empty();
    }

  private:
    std::unordered_map<K, V> m_Data;
    std::mutex m_Mutex;
  };



  template<typename V>
  class UnorderedSetQueue
  {
  public:
    UnorderedSetQueue() = default;

    template<typename T>
    bool add(T&& value)
    {
      std::lock_guard lock(m_Mutex);

      auto [insertionPosition, insertionSuccess] = m_Data.insert(std::forward<T>(value));
      return insertionSuccess;
    }

    template<typename... Args>
    bool emplace(Args&&... args)
    {
      std::lock_guard lock(m_Mutex);

      auto [insertionPosition, insertionSuccess] = m_Data.emplace(std::forward<Args>(args)...);
      return insertionSuccess;
    }

    std::optional<V> tryRemove()
    {
      std::lock_guard lock(m_Mutex);

      if (!m_Data.empty())
      {
        auto nodeHandle = m_Data.extract(m_Data.begin());
        V value = std::move(nodeHandle.value());
        return value;
      }
      return std::nullopt;
    }

    std::size_t empty()
    {
      std::lock_guard lock(m_Mutex);
      return m_Data.empty();
    }

  private:
    std::unordered_set<V> m_Data;
    std::mutex m_Mutex;
  };
}