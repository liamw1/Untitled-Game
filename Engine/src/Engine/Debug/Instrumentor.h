#pragma once

#include <string>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <thread>

namespace Engine
{
  struct ProfileResult
  {
    std::string name;
    int64_t start, end;
    uint32_t threadID;
  };

  struct InstrumentationSession
  {
    std::string name;
  };



  class Instrumentor
  {
  public:
    Instrumentor()
      : m_CurrentSession(nullptr), m_ProfileCount(0)
    {
    }

    void beginSession(const std::string& name, const std::string& filepath = "results.json")
    {
      m_OutputStream.open(filepath);
      writeHeader();
      m_CurrentSession = new InstrumentationSession{ name };
    }

    void endSession()
    {
      writeFooter();
      m_OutputStream.close();
      delete m_CurrentSession;
      m_CurrentSession = nullptr;
      m_ProfileCount = 0;
    }

    void writeProfile(const ProfileResult& result)
    {
      if (m_ProfileCount++ > 0)
        m_OutputStream << ",";

      std::string name = result.name;
      std::replace(name.begin(), name.end(), '"', '\'');

      m_OutputStream << "{";
      m_OutputStream << "\"cat\":\"function\",";
      m_OutputStream << "\"dur\":" << (result.end - result.start) << ',';
      m_OutputStream << "\"name\":\"" << name << "\",";
      m_OutputStream << "\"ph\":\"X\",";
      m_OutputStream << "\"pid\":0,";
      m_OutputStream << "\"tid\":" << result.threadID << ",";
      m_OutputStream << "\"ts\":" << result.start;
      m_OutputStream << "}";

      m_OutputStream.flush();
    }

    void writeHeader()
    {
      m_OutputStream << "{\"otherData\": {},\"traceEvents\":[";
      m_OutputStream.flush();
    }

    void writeFooter()
    {
      m_OutputStream << "]}";
      m_OutputStream.flush();
    }

    static Instrumentor& Get()
    {
      static Instrumentor instance;
      return instance;
    }

  private:
    InstrumentationSession* m_CurrentSession;
    std::ofstream m_OutputStream;
    int m_ProfileCount;
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

      int64_t start = std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch().count();
      int64_t end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();

      uint32_t threadID = std::hash<std::thread::id>{}(std::this_thread::get_id());
      Instrumentor::Get().writeProfile({ m_Name, start, end, threadID });

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
  #define EN_PROFILE_FUNCTION()                     EN_PROFILE_SCOPE(__FUNCSIG__)
#else
  #define EN_PROFILE_BEGIN_SESSION(name, filepath)
  #define EN_PROFILE_END_SESSION()
  #define EN_PROFILE_FUNCTION(name)
  #define EN_PROFILE_SCOPE()
#endif