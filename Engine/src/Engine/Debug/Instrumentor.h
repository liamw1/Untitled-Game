#pragma once
#include "Engine/Core/FixedWidthTypes.h"
#include "Engine/Utilities/Constraints.h"

#define ENG_PROFILE 0
#if ENG_PROFILE
#define ENG_PROFILE_BEGIN_SESSION(name, filepath)  ::eng::debug::Instrumentor::Get().beginSession(name, filepath)
#define ENG_PROFILE_END_SESSION()                  ::eng::debug::Instrumentor::Get().endSession()
#define ENG_PROFILE_SCOPE(name)                    ::eng::debug::InstrumentationTimer timer##__LINE__(name)
#define ENG_PROFILE_FUNCTION()                     ENG_PROFILE_SCOPE(__FUNCTION__)
#else
#define ENG_PROFILE_BEGIN_SESSION(name, filepath)
#define ENG_PROFILE_END_SESSION()
#define ENG_PROFILE_SCOPE(name)
#define ENG_PROFILE_FUNCTION()
#endif

/*
  Tool for easily profiling functions and scopes.
  Outputs formatted .json files for visualization by chrome://tracing.
  To format a function, add ENG_PROFILE_FUNCTION() to top line of desired function.
  To format a scope, use ENG_PROFILE_SCOPE within that scope.
*/
namespace eng::debug
{
  class Instrumentor : private SetInStone
  {
    std::mutex m_Mutex;
    std::optional<std::string> m_CurrentSession;
    std::ofstream m_OutputStream;

  public:
    void beginSession(const std::string& name, const std::string& filepath = "results.json");
    void endSession();

    void writeProfile(const std::string& name, const std::chrono::steady_clock::time_point start, const std::chrono::duration<f64, std::micro> elapsedTime, const std::thread::id threadID);

    static Instrumentor& Get();

  private:
    Instrumentor();
    ~Instrumentor();

    void writeHeader();
    void writeFooter();

    // NOTE: you must already own lock on m_Mutex before calling internalEndSession()
    void internalEndSession();
  };



  class InstrumentationTimer
  {
    const char* m_Name;
    std::chrono::steady_clock::time_point m_StartTimepoint;
    bool m_Stopped;

  public:
    InstrumentationTimer(const char* name);
    ~InstrumentationTimer();

    void stop();
  };
}