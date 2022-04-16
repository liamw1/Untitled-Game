#include "ENpch.h"
#include "Threads.h"

static bool mainThreadSet = false;
static std::thread::id mainThreadID{};

std::thread::id Threads::GetMainThreadID()
{
  EN_CORE_ASSERT(mainThreadID != std::thread::id(), "Main thread has not been set!");
  return mainThreadID;
}

void Threads::SetMainThreadID(std::thread::id threadID)
{
  EN_CORE_ASSERT(mainThreadID == std::thread::id(), "Main thread has already been set!");
  mainThreadID = threadID;
}