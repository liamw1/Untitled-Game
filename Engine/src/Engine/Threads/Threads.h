#pragma once
#include "Engine\Utilities\EnumUtilities.h"

namespace eng::thread
{
  enum class Priority
  {
    Immediate,
    High,
    Normal,
    Low,

    First = 0, Last = Low
  };

  void setAsMainThread();
  bool isMainThread();

  template<typename R>
  bool isReady(const std::future<R>& future)
  {
    using namespace std::chrono_literals;
    return future.wait_for(0s) == std::future_status::ready;
  }
}