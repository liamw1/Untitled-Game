#pragma once

namespace eng
{
  class Timestep
  {
    std::chrono::duration<seconds> m_Timestep;

  public:
    Timestep();
    Timestep(std::chrono::duration<seconds> duration);

    seconds sec() const;
  };
}