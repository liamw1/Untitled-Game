#pragma once
#include "ThreadPool.h"

namespace eng::thread
{
  template<Hashable Identifier, typename ReturnType>
  class WorkSet
  {
    mutable std::mutex m_Mutex;
    std::shared_ptr<ThreadPool> m_ThreadPool;
    std::unordered_set<Identifier> m_Work;
    std::unordered_map<Identifier, std::future<ReturnType>> m_Futures;
    Priority m_Priority;

  public:
    WorkSet() = default;
    WorkSet(const std::shared_ptr<ThreadPool>& threadPool, Priority priority)
      : m_ThreadPool(threadPool), m_Priority(priority) {}

    /*
      Submits a task to the thread pool and returns future immediately.
    */ 
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

    /*
      Submits a task and saves the future. If a task with the same ID has already been saved,
      the old future will be overwritten.
    */
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

    /*
      When the function is called, all saved futures at that particular time are waited on
      until their task is completed. During this time, new tasks can be added to the queue.
      Futures of these tasks are saved and are not waited on or discarded.
    */ 
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

    /*
      Discards all saved futures for finished tasks.
    */ 
    std::vector<Identifier> discardFinished()
    {
      std::vector<Identifier> finishedTasks;

      std::lock_guard lock(m_Mutex);
      for (auto it = m_Futures.begin(); it != m_Futures.end();)
      {
        if (!IsReady(it->second))
        {
          ++it;
          continue;
        }

        finishedTasks.push_back(it->first);
        it = m_Futures.erase(it);
      }
      return finishedTasks;
    }

    /*
      Queries whether a task of the specified ID is in the work queue.
    */ 
    bool contains(const Identifier& id) const
    {
      std::lock_guard lock(m_Mutex);
      return m_Work.contains(id);
    }

    /*
      \returns The number of queued tasks.
    */ 
    uSize queuedTasks() const
    {
      std::lock_guard lock(m_Mutex);
      return m_Work.size();
    }

    /*
      \returns The number of saved futures.
    */ 
    uSize savedResults() const
    {
      std::lock_guard lock(m_Mutex);
      return m_Futures.size();
    }

  private:
    void submitCallback(const Identifier& id)
    {
      std::lock_guard lock(m_Mutex);
      m_Work.erase(id);
    }
  };
}