#include "ENpch.h"
#include "Threads.h"
#include "Engine/Debug/Assert.h"

static std::thread::id mainThreadID;

namespace eng::thread
{
  void setAsMainThread()
  {
    mainThreadID = std::this_thread::get_id();
  }

  bool isMainThread()
  {
    ENG_CORE_ASSERT(mainThreadID != std::thread::id(), "Main thread has not been set!");
    return std::this_thread::get_id() == mainThreadID;
  }
}