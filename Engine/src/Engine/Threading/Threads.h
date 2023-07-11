#pragma once

namespace Engine::Threads
{
  std::thread::id MainThreadID();
  void SetMainThreadID(std::thread::id threadID);
}