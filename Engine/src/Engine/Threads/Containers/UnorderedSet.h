#pragma once

namespace eng::thread
{
  template<Hashable V>
    requires std::movable<V>
  class UnorderedSet : private NonCopyable, NonMovable
  {
    mutable std::shared_mutex m_Mutex;
    std::unordered_set<V> m_Data;

  public:
    UnorderedSet() = default;

    template<DecaysTo<V> T>
    bool insert(T&& value)
    {
      std::lock_guard lock(m_Mutex);

      auto [insertionPosition, insertionSuccess] = m_Data.insert(std::forward<T>(value));
      return insertionSuccess;
    }

    template<DecaysTo<V> T>
    void insertOrReplace(T&& value)
    {
      std::lock_guard lock(m_Mutex);

      m_Data.erase(value);
      m_Data.insert(std::forward<T>(value));
    }

    template<typename... Args>
    bool emplace(Args&&... args)
    {
      std::lock_guard lock(m_Mutex);

      auto [insertionPosition, insertionSuccess] = m_Data.emplace(std::forward<Args>(args)...);
      return insertionSuccess;
    }

    bool erase(const V& value)
    {
      std::lock_guard lock(m_Mutex);
      uSize elementsErased = m_Data.erase(value);
      return elementsErased > 0;
    }

    std::optional<V> tryRemoveAny()
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

    std::unordered_set<V> removeAll()
    {
      std::lock_guard lock(m_Mutex);
      return std::move(m_Data);
    }

    std::unordered_set<V> getCurrentState() const
    {
      std::shared_lock lock(m_Mutex);
      return m_Data;
    }

    bool contains(const V& value) const
    {
      std::shared_lock lock(m_Mutex);
      return m_Data.contains(value);
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
