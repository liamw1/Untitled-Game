#pragma once

namespace eng::threads
{
  enum class Priority
  {
    Immediate,
    High,
    Normal,
    Low,

    First = 0, Last = Low
  };
  static constexpr int c_PriorityCount = 1 + static_cast<int>(Priority::Last) - static_cast<int>(Priority::First);

  void setAsMainThread();
  bool isMainThread();

  template<typename R>
  bool isReady(const std::future<R>& future)
  {
    using namespace std::chrono_literals;
    return future.wait_for(0s) == std::future_status::ready;
  }
}