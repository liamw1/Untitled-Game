#pragma once
#include "Threads.h"
#include "Engine/Utilities/MoveOnlyFunction.h"

namespace Engine::Threads
{
  class ThreadPool
  {
  public:
    ThreadPool(int numThreads);
    ~ThreadPool();

    template<typename F, typename... Args>
      requires std::is_invocable_v<F, Args...>
    std::future<std::invoke_result_t<F, Args...>> submit(Priority priority, F&& function, Args&&... args)
    {
      using ReturnType = std::invoke_result_t<F, Args...>;

      std::packaged_task<ReturnType()> task(std::bind(std::forward<F>(function), std::forward<Args>(args)...));
      std::future<ReturnType> future(task.get_future());
      {
        std::lock_guard lock(m_Mutex);
        m_Work[static_cast<int>(priority)].push(std::move(task));
      }
      m_Condition.notify_one();

      return future;
    }

    size_t queuedTasks() const;

    bool running() const;
    void shutdown();

  private:
    bool m_Stop;
    mutable std::mutex m_Mutex;
    std::condition_variable m_Condition;
    std::vector<std::thread> m_Threads;
    std::array<std::queue<MoveOnlyFunction>, c_PriorityCount> m_Work;

    bool hasWork();

    void workerThread();
  };
}