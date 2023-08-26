#pragma once

namespace Engine::Threads
{
  template<Movable V>
  class UnorderedSetQueue
  {
  public:
    UnorderedSetQueue() = default;

    template<DecaysTo<V> T>
    bool insert(T&& value)
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

    template<DecaysTo<V> T>
    void erase(T&& value)
    {
      std::lock_guard lock(m_Mutex);
      m_Data.erase(value);
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

    bool empty()
    {
      std::lock_guard lock(m_Mutex);
      return m_Data.empty();
    }

    size_t size()
    {
      std::lock_guard lock(m_Mutex);
      return m_Data.size();
    }

    template<typename T>
    bool contains(T&& value)
    {
      std::lock_guard lock(m_Mutex);
      return m_Data.contains(value);
    }

  private:
    std::unordered_set<V> m_Data;
    std::mutex m_Mutex;
  };
}
