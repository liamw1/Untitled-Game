#include "ENpch.h"
#include "ThreadPool.h"
#include "Engine/Core/Logger.h"

namespace eng::thread
{
  static const i32 s_MaxThreads = std::max(2, static_cast<i32>(std::thread::hardware_concurrency() / 2));
  static std::atomic_int s_AlloatedThreads = 0;

  ThreadPool::ThreadPool(const std::string& name, i32 numberOfThreads)
    : m_Stop(false), m_Name(name)
  {
    std::string lastWord = numberOfThreads > 1 ? "threads" : "thread";
    ENG_CORE_TRACE("{0} has allocated {1} {2}", name, numberOfThreads, lastWord);
    s_AlloatedThreads += numberOfThreads;
    if (s_AlloatedThreads > s_MaxThreads)
      ENG_CORE_WARN("Total number of allocated threads exceeds maximum recommended threads for this system!");

    for (i32 i = 0; i < numberOfThreads; ++i)
      m_Threads.emplace_back(&ThreadPool::workerThread, this);
  }

  ThreadPool::ThreadPool(const std::string& name, f64 proportionOfSystemThreads)
    : ThreadPool(name, static_cast<i32>(proportionOfSystemThreads* s_MaxThreads)) {}

  ThreadPool::~ThreadPool()
  {
    shutdown();
    s_AlloatedThreads -= static_cast<i32>(m_Threads.size());
  }

  uSize ThreadPool::queuedTasks() const
  {
    std::lock_guard lock(m_Mutex);
    return algo::reduce(m_Work, [](const std::queue<MoveOnlyFunction>& workQueue) { return workQueue.size(); });
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
