#pragma once

namespace Engine
{
  enum class Priority
  {
    High,
    Normal,
    Low,

    Begin = High, End = Low
  };

  namespace Threads
  {
    template<Movable T>
    class PriorityQueue
    {
    public:
      PriorityQueue() = default;

      void push(T&& value, Priority priority)
      {
        std::lock_guard lock(m_Mutex);

        m_Queues[static_cast<int>(priority)].push(std::forward<T>(value));
      }

      std::optional<T> tryRemove()
      {
        std::lock_guard lock(m_Mutex);

        for (std::queue<T>& queue : m_Queues)
          if (!queue.empty())
          {
            T value = std::move(queue.front());
            queue.pop();
            return value;
          }
        return std::nullopt;
      }

    private:
      static constexpr int c_PriorityCount = static_cast<int>(Priority::End) - static_cast<int>(Priority::Begin);

      std::array<std::queue<T>, c_PriorityCount> m_Queues;
      std::mutex m_Mutex;
    };
  }
}