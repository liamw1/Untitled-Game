#pragma once
#include "Threads.h"
#include "Engine/Utilities/EnumUtilities.h"
#include "Engine/Utilities/MoveOnlyFunction.h"

namespace eng::threads
{
  class ThreadPool
  {
  public:
    ThreadPool(int numThreads);
    ~ThreadPool();

    /*
      Submits a task to the thread pool at some specified priority.
      If reference semantics is desired, you must wrap arguments in std::ref.
    */
    template<typename F, typename... Args>
      requires std::is_invocable_v<F, Args...>
    std::future<std::invoke_result_t<F, Args...>> submit(Priority priority, F&& function, Args&&... args)
    {
      using ResultType = std::invoke_result_t<F, Args...>;

      // std::bind makes copies when arguments are references, which is necessary as arguments may be out of scope when task is executed
      std::packaged_task<ResultType()> task(std::bind(std::forward<F>(function), std::forward<Args>(args)...));
      std::future<ResultType> future(task.get_future());
      {
        std::lock_guard lock(m_Mutex);
        m_Work[priority].push(std::move(task));
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
    EnumArray<std::queue<MoveOnlyFunction>, Priority> m_Work;

    bool hasWork();

    void workerThread();
  };
}