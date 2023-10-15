#pragma once

namespace eng::threads
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

  void setAsMainThread();
  bool isMainThread();

  template<typename R>
  bool isReady(const std::future<R>& future)
  {
    using namespace std::chrono_literals;
    return future.wait_for(0s) == std::future_status::ready;
  }
}