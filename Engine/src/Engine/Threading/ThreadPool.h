#pragma once
#include "Threads.h"
#include "Engine/Utilities/MoveableFunction.h"

#include <future>
#include <mutex>
#include <queue>

namespace Engine::Threads
{
  class ThreadPool
  {
  public:
    ThreadPool(int numThreads);
    ~ThreadPool();

    template<std::invocable<> F>
    std::future<std::invoke_result_t<F>> submit(F function, Priority priority)
    {
      using ReturnType = std::invoke_result_t<F>;

      std::packaged_task<ReturnType()> task(std::move(function));
      std::future<ReturnType> future(task.get_future());
      {
        std::lock_guard lock(m_Mutex);
        m_Work[static_cast<int>(priority)].emplace(std::move(task));
      }
      m_Condition.notify_one();

      return future;
    }

  private:
    bool m_Stop;
    std::mutex m_Mutex;
    std::condition_variable m_Condition;
    std::vector<std::thread> m_Threads;
    std::array<std::queue<MoveableFunction>, c_PriorityCount> m_Work;

    bool hasWork();

    void workerThread();
  };
}