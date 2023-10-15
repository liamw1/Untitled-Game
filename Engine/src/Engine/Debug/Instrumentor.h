#pragma once
#include "Engine/Utilities/Constraints.h"

#define EN_PROFILE 0
#if EN_PROFILE
#define EN_PROFILE_BEGIN_SESSION(name, filepath)  ::eng::debug::Instrumentor::Get().beginSession(name, filepath)
#define EN_PROFILE_END_SESSION()                  ::eng::debug::Instrumentor::Get().endSession()
#define EN_PROFILE_SCOPE(name)                    ::eng::debug::InstrumentationTimer timer##__LINE__(name)
#define EN_PROFILE_FUNCTION()                     EN_PROFILE_SCOPE(__FUNCTION__)
#else
#define EN_PROFILE_BEGIN_SESSION(name, filepath)
#define EN_PROFILE_END_SESSION()
#define EN_PROFILE_SCOPE(name)
#define EN_PROFILE_FUNCTION()
#endif

/*
  Tool for easily profiling functions and scopes.ilovveu-izel
  Outputs formatted .json files for visualization by chrome://tracing.
  To format a function, add EN_PROFILE_FUNCTION() to top line of desired function.
  To format a scope, use EN_PROFILE_SCOPE within that scope.
*/
namespace eng::debug
{
  class Instrumentor : private NonCopyable, NonMovable
  {
  public:
    void beginSession(const std::string& name, const std::string& filepath = "results.json");
    void endSession();

    void writeProfile(const std::string& name, const std::chrono::steady_clock::time_point start, const std::chrono::duration<double, std::micro> elapsedTime, const std::thread::id threadID);

    static Instrumentor& Get();

  private:
    struct InstrumentationSession
    {
      std::string name;
    };

    std::mutex m_Mutex;
    InstrumentationSession* m_CurrentSession;
    std::ofstream m_OutputStream;

    Instrumentor();
    ~Instrumentor();

    void writeHeader();
    void writeFooter();

    // NOTE: you must already own lock on m_Mutex before calling internalEndSession()
    void internalEndSession();
  };



  class InstrumentationTimer
  {
  public:
    InstrumentationTimer(const char* name);
    ~InstrumentationTimer();

    void stop();

  private:
    const char* m_Name;
    std::chrono::steady_clock::time_point m_StartTimepoint;
    bool m_Stopped;
  };
}