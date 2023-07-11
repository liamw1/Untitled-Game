#include "ENpch.h"
#include "Timer.h"

Timer::Timer()
  : Timer("Unnamed Timer") {}

Timer::Timer(const char* name)
  : m_Name(name), m_Duration(std::chrono::high_resolution_clock::duration::zero()) {}

Timer::~Timer()
{
  std::cout << m_Name << " took " << m_Duration.count() * 1000.0f << "ms" << std::endl;
}

void Timer::timeStart()
{
  m_Start = std::chrono::high_resolution_clock::now();
}

void Timer::timeStop()
{
  m_End = std::chrono::high_resolution_clock::now();
  m_Duration += m_End - m_Start;
}