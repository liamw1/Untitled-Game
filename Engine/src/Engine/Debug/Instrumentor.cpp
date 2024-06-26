#include "ENpch.h"
#include "Instrumentor.h"
#include "Engine/Core/Casting.h"

namespace eng::debug
{
  void Instrumentor::beginSession(std::string_view name, std::filesystem::path filepath)
  {
    std::lock_guard lock(m_Mutex);

    /*
      If there is already a current session, then close it before beginning new one.
      Subsequent profiling output meant for the original session will end up in the
      newly opened session instead.  That's better than having badly formatted
      profiling output.
    */
    if (m_CurrentSession)
      internalEndSession();

    m_OutputStream.open(filepath);
    if (m_OutputStream.is_open())
    {
      m_CurrentSession = name;
      writeHeader();
    }
  }

  void Instrumentor::endSession()
  {
    std::lock_guard lock(m_Mutex);
    internalEndSession();
  }

  void Instrumentor::writeProfile(std::string_view name, std::chrono::steady_clock::time_point start, std::chrono::duration<f64, std::micro> elapsedTime, std::thread::id threadID)
  {
    i64 startTimeInNanoseconds = std::chrono::time_point_cast<std::chrono::nanoseconds>(start).time_since_epoch().count();

    std::stringstream json{};

    json << std::setprecision(3) << std::fixed;
    json << ",{";
    json << "\"cat\":\"function\",";
    json << "\"dur\":" << elapsedTime.count() << ',';
    json << "\"name\":\"" << name << "\",";
    json << "\"ph\":\"X\",";
    json << "\"pid\":0,";
    json << "\"tid\":" << threadID << ",";
    json << "\"ts\":" << arithmeticUpcast<f64>(startTimeInNanoseconds) / 1e3;
    json << "}";

    if (m_CurrentSession)
    {
      std::lock_guard lock(m_Mutex);

      m_OutputStream << json.str();
      m_OutputStream.flush();
    }
  }

  Instrumentor& Instrumentor::Get()
  {
    static Instrumentor instance;
    return instance;
  }

  Instrumentor::Instrumentor()
    : m_CurrentSession(std::nullopt) {}

  Instrumentor::~Instrumentor()
  {
    endSession();
  }

  void Instrumentor::writeHeader()
  {
    m_OutputStream << "{\"otherData\": {},\"traceEvents\":[{}";
    m_OutputStream.flush();
  }

  void Instrumentor::writeFooter()
  {
    m_OutputStream << "]}";
    m_OutputStream.flush();
  }

  void Instrumentor::internalEndSession()
  {
    if (m_CurrentSession)
    {
      writeFooter();
      m_OutputStream.close();
      m_CurrentSession.reset();
    }
  }

  InstrumentationTimer::InstrumentationTimer(std::string_view name)
    : m_Name(name), m_StartTimepoint(std::chrono::steady_clock::now()), m_Stopped(false) {}

  InstrumentationTimer::~InstrumentationTimer()
  {
    if (!m_Stopped)
      stop();
  }

  void InstrumentationTimer::stop()
  {
    auto endTimepoint = std::chrono::steady_clock::now();
    std::chrono::duration<f64, std::micro> elapsedTime = endTimepoint - m_StartTimepoint;
    Instrumentor::Get().writeProfile(m_Name, m_StartTimepoint, elapsedTime, std::this_thread::get_id());

    m_Stopped = true;
  }
}