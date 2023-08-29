#pragma once

namespace Engine::Threads
{
  template<Movable V>
    requires Hashable<V>
  class UnorderedSet
  {
  public:
    UnorderedSet() = default;

    template<DecaysTo<V> T>
    bool insert(T&& value)
    {
      std::lock_guard lock(m_Mutex);

      auto [insertionPosition, insertionSuccess] = m_Data.insert(std::forward<T>(value));
      return insertionSuccess;
    }

    void insertAll(const std::vector<V>& values)
    {
      std::lock_guard lock(m_Mutex);

      for (const V& value : values)
        m_Data.insert(value);
    }

    template<typename... Args>
    bool emplace(Args&&... args)
    {
      std::lock_guard lock(m_Mutex);

      auto [insertionPosition, insertionSuccess] = m_Data.emplace(std::forward<Args>(args)...);
      return insertionSuccess;
    }

    void erase(const V& value)
    {
      std::lock_guard lock(m_Mutex);
      m_Data.erase(value);
    }

    void eraseAll(const std::vector<V>& values)
    {
      std::lock_guard lock(m_Mutex);

      for (const V& value : values)
        m_Data.erase(value);
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
      std::lock_guard lock(m_Mutex);
      return m_Data;
    }

    bool contains(const V& value) const
    {
      std::lock_guard lock(m_Mutex);
      return m_Data.contains(value);
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
    std::unordered_set<V> m_Data;
  };
}
