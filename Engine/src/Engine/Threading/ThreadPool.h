#pragma once
#include "Containers/PriorityQueue.h"
#include "Engine/Utilities/MoveableFunction.h"

#include <atomic>
#include <future>

namespace Engine::Threads
{
  class ThreadPool
  {
  public:
    ThreadPool(int numThreads)
      : m_Done(false)
    {
      for (int i = 0; i < numThreads; ++i)
        m_Threads.emplace_back(&ThreadPool::workerThread, this);
    }

    ~ThreadPool()
    {
      m_Done = true;

      for (std::thread& thread : m_Threads)
        if (thread.joinable())
          thread.join();
    }

    template<std::invocable<> F>
    std::future<std::invoke_result_t<F>> submit(F function, Priority priority)
    {
      std::packaged_task<std::invoke_result_t<F>()> task(std::move(function));
      std::future<std::invoke_result_t<F>> future(task.get_future());
      m_WorkQueue.push(std::move(task), priority);
      return future;
    }

  private:
    std::atomic_bool m_Done;
    std::vector<std::thread> m_Threads;
    PriorityQueue<MoveableFunction> m_WorkQueue;

    void workerThread()
    {
      using namespace std::chrono_literals;

      while (!m_Done)
      {
        EN_PROFILE_SCOPE("Worker Loop");

        std::optional<MoveableFunction> task = m_WorkQueue.tryRemove();
        if (task)
          (*task)();
        else
          std::this_thread::sleep_for(10ms);
      }
    }
  };
}