#pragma once

class Timer
{
public:
  Timer()
    : Timer("Unnamed Timer") {}
  ~Timer();

  Timer(const char* name);

  void timeStart();
  void timeStop();

private:
  std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
  std::chrono::duration<float> duration;
  std::string timerName;
};