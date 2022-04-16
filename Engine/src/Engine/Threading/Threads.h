#pragma once

namespace Threads
{
  std::thread::id GetMainThreadID();
  void SetMainThreadID(std::thread::id threadID);
}