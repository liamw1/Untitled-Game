#pragma once

#include <codeanalysis\warnings.h> // Disable intellisense warnings
#pragma warning(push, 0)
#pragma warning(disable : ALL_CODE_ANALYSIS_WARNINGS)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#pragma warning(pop)

namespace Engine
{
  class Log
  {
  public:
    static void Initialize();

    static Shared<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
    static Shared<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

  private:
    static Shared<spdlog::logger> s_CoreLogger;
    static Shared<spdlog::logger> s_ClientLogger;
  };
}

// Core log macros
#define EN_CORE_TRACE(...)   ::Engine::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define EN_CORE_INFO(...)    ::Engine::Log::GetCoreLogger()->info(__VA_ARGS__)
#define EN_CORE_WARN(...)    ::Engine::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define EN_CORE_ERROR(...)   ::Engine::Log::GetCoreLogger()->error(__VA_ARGS__)
#define EN_CORE_FATAL(...)   ::Engine::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define EN_TRACE(...)        ::Engine::Log::GetClientLogger()->trace(__VA_ARGS__)
#define EN_INFO(...)         ::Engine::Log::GetClientLogger()->info(__VA_ARGS__)
#define EN_WARN(...)         ::Engine::Log::GetClientLogger()->warn(__VA_ARGS__)
#define EN_ERROR(...)        ::Engine::Log::GetClientLogger()->error(__VA_ARGS__)
#define EN_FATAL(...)        ::Engine::Log::GetClientLogger()->critical(__VA_ARGS__)

// Assert macros.  Will be removed from code in non-debug builds
#ifdef EN_ENABLE_ASSERTS
  #define EN_ASSERT(x, ...) { if (!(x)) { EN_ERROR(__VA_ARGS__); EN_DEBUG_BREAK(); } }
  #define EN_CORE_ASSERT(x, ...) { if (!(x)) { EN_CORE_ERROR(__VA_ARGS__); EN_DEBUG_BREAK(); } }
#else
  #define EN_ASSERT(x, ...)
  #define EN_CORE_ASSERT(x, ...)
#endif
