#pragma once

#include "ThreadPool.h"
#include "Engine/Core/Concepts.h"
#include <memory>

namespace Engine::Threads
{
  template<Hashable Identifier, typename ReturnType>
  class WorkSet
  {
  public:
    WorkSet(const std::shared_ptr<ThreadPool>& threadPool, Priority priority)
      : m_ThreadPool(threadPool), m_Priority(priority) {}

    template<InvocableWithReturnType<ReturnType> F>
    std::future<ReturnType> submit(const Identifier& id, F function)
    {
      {
        std::lock_guard lock(m_Mutex);

        auto [insertionPosition, insertionSuccess] = m_Work.insert(id);
        if (!insertionSuccess)
          return {};
      }

      return m_ThreadPool->submit([this, id, function]()
        {
          this->submitCallback(id);
          return function();
        }, m_Priority);
    }

    template<InvocableWithReturnType<ReturnType> F>
    void submitAndSaveResult(const Identifier& id, F function)
    {
      std::future<ReturnType> future = submit(id, function);
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
        future.get();
    }

    bool contains(const Identifier& id)
    {
      std::lock_guard lock(m_Mutex);
      return m_Work.contains(id);
    }

  private:
    std::mutex m_Mutex;
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