#pragma once
#include "Engine/Utilities/Constraints.h"

namespace eng::thread
{
  template<std::movable V>
  class Queue : private SetInStone
  {
    std::mutex m_Mutex;
    std::queue<V> m_Data;

  public:
    Queue() = default;

    template<DecaysTo<V> T>
    void push(T&& value)
    {
      std::lock_guard lock(m_Mutex);
      m_Data.push(std::forward<T>(value));
    }

    template<typename... Args>
      requires std::constructible_from<V, Args...>
    void emplace(Args&&... args)
    {
      std::lock_guard lock(m_Mutex);
      m_Data.emplace(std::forward<Args>(args)...);
    }

    std::optional<V> tryPop()
    {
      std::lock_guard lock(m_Mutex);

      if (m_Data.empty())
        return std::nullopt;

      std::optional<V> frontElement = std::move(m_Data.front());
      m_Data.pop();
      return frontElement;
    }
  };
}