#pragma once
#include "Core.h"
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace Engine
{
  class ENGINE_API Log
  {
  public:
    static void Initialize();

    inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return CoreLogger; }
    inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return ClientLogger; }

  private:
    static std::shared_ptr<spdlog::logger> CoreLogger;
    static std::shared_ptr<spdlog::logger> ClientLogger;
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
  #define EN_ASSERT(x, ...) { if (!(x)) { EN_ERROR("Assertion Failed: {0}", __VA_ARGS__); EN_DEBUG_BREAK(); } }
  #define EN_CORE_ASSERT(x, ...) { if (!(x)) { EN_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); EN_DEBUG_BREAK(); } }
#else
  #define EN_ASSERT(x, ...)
  #define EN_CORE_ASSERT(x, ...)
#endif
