#pragma once
#include "Threads.h"
#include "Engine/Utilities/EnumUtilities.h"
#include "Engine/Utilities/MoveOnlyFunction.h"

namespace eng::thread
{
  class ThreadPool
  {
    bool m_Stop;
    mutable std::mutex m_Mutex;
    std::condition_variable m_Condition;
    std::vector<std::thread> m_Threads;
    EnumArray<std::queue<MoveOnlyFunction>, Priority> m_Work;
    std::string m_Name;

  public:
    ThreadPool(const std::string& name, i32 numberOfThreads);
    ThreadPool(const std::string& name, f64 proportionOfSystemThreads);
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

    uSize queuedTasks() const;

    bool running() const;
    void shutdown();

  private:
    using WorkIterator = decltype(m_Work)::iterator;

    bool hasWork();
    WorkIterator firstQueueWithWork();

    void workerThread();
  };
}