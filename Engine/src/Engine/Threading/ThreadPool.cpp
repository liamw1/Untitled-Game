#include "ENpch.h"
#include "ThreadPool.h"
#include "Engine/Debug/Instrumentor.h"

namespace Engine
{
  Threads::ThreadPool::ThreadPool(int numThreads)
    : m_Stop(false)
  {
    for (int i = 0; i < numThreads; ++i)
      m_Threads.emplace_back(&ThreadPool::workerThread, this);
  }

  Threads::ThreadPool::~ThreadPool()
  {
    shutdown();
  }

  size_t Threads::ThreadPool::queuedTasks() const
  {
    std::lock_guard lock(m_Mutex);

    size_t queuedTasks = 0;
    for (const std::queue<MoveOnlyFunction>& workQueue : m_Work)
      queuedTasks += workQueue.size();
    return queuedTasks;
  }

  bool Threads::ThreadPool::running() const
  {
    std::lock_guard lock(m_Mutex);
    return !m_Stop;
  }

  void Threads::ThreadPool::shutdown()
  {
    {
      std::lock_guard lock(m_Mutex);
      m_Stop = true;
    }
    m_Condition.notify_all();

    for (std::thread& thread : m_Threads)
      if (thread.joinable())
        thread.join();
  }

  bool Threads::ThreadPool::hasWork()
  {
    for (const std::queue<MoveOnlyFunction>& workQueue : m_Work)
      if (!workQueue.empty())
        return true;
    return false;
  }

  void Threads::ThreadPool::workerThread()
  {
    while (true)
    {
      MoveOnlyFunction task;

      {
        std::unique_lock lock(m_Mutex);
        m_Condition.wait(lock, [this] { return m_Stop || hasWork(); });

        if (m_Stop)
          return;

        for (std::queue<MoveOnlyFunction>& workQueue : m_Work)
          if (!workQueue.empty())
          {
            task = std::move(workQueue.front());
            workQueue.pop();
            break;
          }
      }

      task();
    }
  }
}
