#pragma once
#include "Engine/Core/Log.h"

namespace Engine
{
  struct ProfileResult
  {
    std::string name;
    std::chrono::steady_clock::time_point start;
    std::chrono::duration<double, std::nano> elapsedTime;
    std::thread::id threadID;
  };

  struct InstrumentationSession
  {
    std::string name;
  };



  class Instrumentor
  {
  public:
    Instrumentor(const Instrumentor&) = delete;
    Instrumentor(Instrumentor&&) = delete;

    void beginSession(const std::string& name, const std::string& filepath = "results.json")
    {
      std::lock_guard lock(m_Mutex);
      if (m_CurrentSession)
      {
        /*
          If there is already a current session, then close it before beginning new one.
          Subsequent profiling output meant for the original session will end up in the
          newly opened session instead.  That's better than having badly formatted
          profiling output.
        */
        if (Log::GetCoreLogger()) // Edge case: BeginSession() might be before Log::Init()
          EN_CORE_ERROR("Instrumentor::BeginSession('{0}') when session '{1}' already open.", name, m_CurrentSession->name);

        internalEndSession();
      }

      m_OutputStream.open(filepath);
      if (m_OutputStream.is_open())
      {
        m_CurrentSession = new InstrumentationSession({ name });
        writeHeader();
      }
      else if (Log::GetCoreLogger()) // Edge case: BeginSession() might be before Log::Init()
        EN_CORE_ERROR("Instrumentor could not open results file '{0}'.", filepath);
    }

    void endSession()
    {
      std::lock_guard lock(m_Mutex);
      internalEndSession();
    }

    void writeProfile(const ProfileResult& result)
    {
      m_OutputStream << ",{";
      m_OutputStream << "\"cat\":\"function\",";
      m_OutputStream << "\"dur\":" << result.elapsedTime.count() << ',';
      m_OutputStream << "\"name\":\"" << result.name << "\",";
      m_OutputStream << "\"ph\":\"X\",";
      m_OutputStream << "\"pid\":0,";
      m_OutputStream << "\"tid\":" << result.threadID << ",";
      m_OutputStream << "\"ts\":" << std::chrono::time_point_cast<std::chrono::nanoseconds>(result.start).time_since_epoch().count();
      m_OutputStream << "}";

      std::lock_guard lock(m_Mutex);
      if (m_CurrentSession)
        m_OutputStream.flush();
    }

    static Instrumentor& Get()
    {
      static Instrumentor instance;
      return instance;
    }

  private:
    std::mutex m_Mutex;
    InstrumentationSession* m_CurrentSession;
    std::ofstream m_OutputStream;

    Instrumentor()
      : m_CurrentSession(nullptr) {}

    ~Instrumentor() { endSession(); }

    void writeHeader()
    {
      m_OutputStream << "{\"otherData\": {},\"traceEvents\":[{}";
      m_OutputStream.flush();
    }

    void writeFooter()
    {
      m_OutputStream << "]}";
      m_OutputStream.flush();
    }

    // Note: you must already own lock on m_Mutex before calling internalEndSession()
    void internalEndSession()
    {
      if (m_CurrentSession)
      {
        writeFooter();
        m_OutputStream.close();
        delete m_CurrentSession;
        m_CurrentSession = nullptr;
      }
    }
  };



  class InstrumentationTimer
  {
  public:
    InstrumentationTimer(const char* name)
      : m_Name(name), m_StartTimepoint(std::chrono::steady_clock::now()), m_Stopped(false)
    {
    }

    ~InstrumentationTimer()
    {
      if (!m_Stopped)
        stop();
    }

    void stop()
    {
      auto endTimepoint = std::chrono::steady_clock::now();
      std::chrono::duration<double, std::nano> elapsedTime = endTimepoint - m_StartTimepoint;
      Instrumentor::Get().writeProfile({ m_Name, m_StartTimepoint, elapsedTime, std::this_thread::get_id() });

      m_Stopped = true;
    }

  private:
    const char* m_Name;
    std::chrono::steady_clock::time_point m_StartTimepoint;
    bool m_Stopped;
  };
}

#define EN_PROFILE 1
#if EN_PROFILE
#define EN_PROFILE_BEGIN_SESSION(name, filepath)  ::Engine::Instrumentor::Get().beginSession(name, filepath)
#define EN_PROFILE_END_SESSION()                  ::Engine::Instrumentor::Get().endSession()
#define EN_PROFILE_SCOPE(name)                    ::Engine::InstrumentationTimer timer##__LINE__(name)
#define EN_PROFILE_FUNCTION()                     EN_PROFILE_SCOPE(__FUNCTION__)
#else
#define EN_PROFILE_BEGIN_SESSION(name, filepath)
#define EN_PROFILE_END_SESSION()
#define EN_PROFILE_FUNCTION(name)
#define EN_PROFILE_SCOPE()
#endif