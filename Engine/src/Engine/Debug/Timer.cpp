#include "ENpch.h"
#include "Timer.h"

Timer::~Timer()
{
  std::cout << timerName << " took " << duration.count() * 1000.0f << "ms" << std::endl;
}

Timer::Timer(const char* name)
  : timerName(name)
{
  duration = std::chrono::high_resolution_clock::duration::zero();
}

void Timer::timeStart()
{
  start = std::chrono::high_resolution_clock::now();
}

void Timer::timeStop()
{
  end = std::chrono::high_resolution_clock::now();
  duration += end - start;
}