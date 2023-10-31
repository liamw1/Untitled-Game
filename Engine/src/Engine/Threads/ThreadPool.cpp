#include "ENpch.h"
#include "ThreadPool.h"

namespace eng::threads
{
  ThreadPool::ThreadPool(i32 numThreads)
    : m_Stop(false)
  {
    for (i32 i = 0; i < numThreads; ++i)
      m_Threads.emplace_back(&ThreadPool::workerThread, this);
  }

  ThreadPool::~ThreadPool()
  {
    shutdown();
  }

  size_t ThreadPool::queuedTasks() const
  {
    std::lock_guard lock(m_Mutex);
    return algo::sum(m_Work, [](const std::queue<MoveOnlyFunction>& workQueue) { return workQueue.size(); }, size_t());
  }

  bool ThreadPool::running() const
  {
    std::lock_guard lock(m_Mutex);
    return !m_Stop;
  }

  void ThreadPool::shutdown()
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

  bool ThreadPool::hasWork()
  {
    return firstQueueWithWork() != m_Work.end();
  }

  ThreadPool::WorkIterator ThreadPool::firstQueueWithWork()
  {
    return algo::findIf(m_Work, [](const std::queue<MoveOnlyFunction>& workQueue) { return !workQueue.empty(); });
  }

  void ThreadPool::workerThread()
  {
    while (true)
    {
      MoveOnlyFunction task;

      {
        std::unique_lock lock(m_Mutex);
        m_Condition.wait(lock, [this] { return m_Stop || hasWork(); });

        if (m_Stop)
          return;

        WorkIterator workQueuePosition = firstQueueWithWork();
        if (workQueuePosition == m_Work.end())
          continue;

        task = std::move(workQueuePosition->front());
        workQueuePosition->pop();
      }

      task();
    }
  }
}
