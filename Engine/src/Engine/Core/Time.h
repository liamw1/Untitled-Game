#pragma once

namespace Engine
{
  class Timestep
  {
  public:
    Timestep();
    Timestep(std::chrono::duration<seconds> duration);

    seconds sec() const;

  private:
    std::chrono::duration<seconds> m_Timestep;
  };
}