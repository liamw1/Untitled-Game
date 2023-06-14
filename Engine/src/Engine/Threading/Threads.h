#pragma once

namespace Threads
{
  std::thread::id MainThreadID();
  void SetMainThreadID(std::thread::id threadID);
}