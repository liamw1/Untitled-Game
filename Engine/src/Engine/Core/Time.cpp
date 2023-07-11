#include "ENpch.h"
#include "Time.h"

namespace Engine
{
  Timestep::Timestep()
    : m_Timestep(std::chrono::duration<seconds>::zero()) {}
  Timestep::Timestep(std::chrono::duration<seconds> duration)
    : m_Timestep(duration) {}

  seconds Timestep::sec() const
  {
    return m_Timestep.count();
  }
}