#include "ENpch.h"
#include "Timer.h"
#include "Engine/Core/Logger.h"

namespace eng::debug
{
  Timer::Timer()
  : Timer("Unnamed Timer") {}
  
  Timer::Timer(const char* name)
    : m_Name(name), m_Duration(std::chrono::high_resolution_clock::duration::zero()) {}
  
  Timer::~Timer()
  {
    ENG_CORE_INFO("{0} took {1} ms", m_Name, m_Duration.count() * 1000.0f);
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
}