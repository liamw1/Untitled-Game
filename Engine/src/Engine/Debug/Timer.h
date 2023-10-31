#pragma once

namespace eng::debug
{
  class Timer
  {
  public:
    Timer();
    Timer(const char* name);
    ~Timer();

    void timeStart();
    void timeStop();

  private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_End;
    std::chrono::duration<f32> m_Duration;
    std::string m_Name;
  };
}