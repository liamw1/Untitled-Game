#include "ENpch.h"
#include "Threads.h"

static std::thread::id mainThreadID;

namespace Engine
{
  void Threads::SetAsMainThread()
  {
    mainThreadID = std::this_thread::get_id();
  }

  bool Threads::IsMainThread()
  {
    EN_CORE_ASSERT(mainThreadID != std::thread::id(), "Main thread has not been set!");
    return std::this_thread::get_id() == mainThreadID;
  }
}