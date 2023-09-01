#pragma once

namespace Engine::Threads
{
  enum class Priority
  {
    High,
    Normal,
    Low,

    Begin = High, End = Low
  };
  static constexpr int c_PriorityCount = 1 + static_cast<int>(Priority::End) - static_cast<int>(Priority::Begin);

  void SetAsMainThread();
  bool IsMainThread();
}