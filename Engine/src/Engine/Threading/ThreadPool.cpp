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
    for (const std::queue<MoveableFunction>& workQueue : m_Work)
      if (!workQueue.empty())
        return true;
    return false;
  }

  void Threads::ThreadPool::workerThread()
  {
    while (true)
    {
      MoveableFunction task;

      {
        std::unique_lock lock(m_Mutex);
        m_Condition.wait(lock, [this] { return m_Stop || hasWork(); });

        if (m_Stop)
          return;

        for (std::queue<MoveableFunction>& workQueue : m_Work)
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
