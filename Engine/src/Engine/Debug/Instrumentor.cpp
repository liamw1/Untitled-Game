#include "ENpch.h"
#include "Instrumentor.h"

namespace Engine
{
  void Instrumentor::beginSession(const std::string& name, const std::string& filepath)
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

  void Instrumentor::endSession()
  {
    std::lock_guard lock(m_Mutex);
    internalEndSession();
  }

  void Instrumentor::writeProfile(const std::string& name, const std::chrono::steady_clock::time_point start, const std::chrono::duration<double, std::micro> elapsedTime, const std::thread::id threadID)
  {
    int64_t startTimeInNanoseconds = std::chrono::time_point_cast<std::chrono::nanoseconds>(start).time_since_epoch().count();

    std::stringstream json{};

    json << std::setprecision(3) << std::fixed;
    json << ",{";
    json << "\"cat\":\"function\",";
    json << "\"dur\":" << elapsedTime.count() << ',';
    json << "\"name\":\"" << name << "\",";
    json << "\"ph\":\"X\",";
    json << "\"pid\":0,";
    json << "\"tid\":" << threadID << ",";
    json << "\"ts\":" << static_cast<double>(startTimeInNanoseconds) / 1e3;
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
    : m_CurrentSession(nullptr) {}

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
      delete m_CurrentSession;
      m_CurrentSession = nullptr;
    }
  }

  InstrumentationTimer::InstrumentationTimer(const char* name)
    : m_Name(name), m_StartTimepoint(std::chrono::steady_clock::now()), m_Stopped(false) {}

  InstrumentationTimer::~InstrumentationTimer()
  {
    if (!m_Stopped)
      stop();
  }

  void InstrumentationTimer::stop()
  {
    auto endTimepoint = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::micro> elapsedTime = endTimepoint - m_StartTimepoint;
    Instrumentor::Get().writeProfile(m_Name, m_StartTimepoint, elapsedTime, std::this_thread::get_id());

    m_Stopped = true;
  }
}