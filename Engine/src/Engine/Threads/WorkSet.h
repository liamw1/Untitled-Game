#pragma once
#include "ThreadPool.h"

namespace Engine::Threads
{
  template<Hashable Identifier, typename ReturnType>
  class WorkSet
  {
  public:
    WorkSet() = default;
    WorkSet(const std::shared_ptr<ThreadPool>& threadPool, Priority priority)
      : m_ThreadPool(threadPool), m_Priority(priority) {}

    template<typename F, typename... Args>
      requires std::is_invocable_r_v<ReturnType, F, Args...>
    std::future<ReturnType> submit(const Identifier& id, F&& function, Args&&... args)
    {
      {
        std::lock_guard lock(m_Mutex);

        auto [insertionPosition, insertionSuccess] = m_Work.insert(id);
        if (!insertionSuccess)
          return {};
      }

      return m_ThreadPool->submit(m_Priority, [this, id]<typename F, typename... Args>(F&& f, Args&&... a)
      {
        this->submitCallback(id);
        return std::invoke(std::forward<F>(f), std::forward<Args>(a)...);
      }, std::forward<F>(function), std::forward<Args>(args)...);
    }

    template<typename F, typename... Args>
      requires std::is_invocable_r_v<ReturnType, F, Args...>
    void submitAndSaveResult(const Identifier& id, F&& function, Args&&... args)
    {
      std::future<ReturnType> future = submit(id, std::forward<F>(function), std::forward<Args>(args)...);
      if (future.valid())
      {
        std::lock_guard lock(m_Mutex);
        m_Futures.emplace(id, std::move(future));
      }
    }

    void waitAndDiscardSaved()
    {
      std::unordered_map<Identifier, std::future<ReturnType>> futures;

      {
        std::lock_guard lock(m_Mutex);
        futures = std::move(m_Futures);
      }

      for (auto& [id, future] : futures)
        future.wait();
    }

    bool contains(const Identifier& id) const
    {
      std::lock_guard lock(m_Mutex);
      return m_Work.contains(id);
    }

    size_t queuedTasks() const
    {
      std::lock_guard lock(m_Mutex);
      return m_Work.size();
    }

    size_t savedResults() const
    {
      std::lock_guard lock(m_Mutex);
      return m_Futures.size();
    }

  private:
    mutable std::mutex m_Mutex;
    std::shared_ptr<ThreadPool> m_ThreadPool;
    std::unordered_set<Identifier> m_Work;
    std::unordered_map<Identifier, std::future<ReturnType>> m_Futures;
    Priority m_Priority;

    void submitCallback(const Identifier& id)
    {
      std::lock_guard lock(m_Mutex);
      m_Work.erase(id);
    }
  };
}