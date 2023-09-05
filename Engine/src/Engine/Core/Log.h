#pragma once

namespace Engine
{
  class Log
  {
  public:
    static void Initialize();

    static std::shared_ptr<spdlog::logger>& GetCoreLogger();
    static std::shared_ptr<spdlog::logger>& GetClientLogger();

  private:
    static std::shared_ptr<spdlog::logger> s_CoreLogger;
    static std::shared_ptr<spdlog::logger> s_ClientLogger;
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
