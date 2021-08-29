#include "ENpch.h"
#include "Log.h"

namespace Engine
{
  std::shared_ptr<spdlog::logger> Log::CoreLogger;
  std::shared_ptr<spdlog::logger> Log::ClientLogger;

  void Log::Initialize()
  {
    spdlog::set_pattern("%^[%T] %n: %v%$");

    CoreLogger = spdlog::stdout_color_mt("ENGINE");
    CoreLogger->set_level(spdlog::level::trace);

    ClientLogger = spdlog::stdout_color_mt("APP");
    ClientLogger->set_level(spdlog::level::trace);
  }
}