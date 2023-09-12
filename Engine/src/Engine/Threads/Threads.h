#pragma once

namespace Engine::Threads
{
  enum class Priority
  {
    Immediate,
    High,
    Normal,
    Low,

    Begin = 0, End = Low
  };
  static constexpr int c_PriorityCount = 1 + static_cast<int>(Priority::End) - static_cast<int>(Priority::Begin);

  void SetAsMainThread();
  bool IsMainThread();

  template<typename R>
  bool IsReady(const std::future<R>& future)
  {
    using namespace std::chrono_literals;
    return future.wait_for(0s) == std::future_status::ready;
  }
}