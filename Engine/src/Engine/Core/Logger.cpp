#include "ENpch.h"
#include "Logger.h"

namespace eng::log
{
  struct Loggers
  {
    std::shared_ptr<spdlog::logger> core;
    std::shared_ptr<spdlog::logger> client;

    Loggers()
    {
      spdlog::set_pattern("%^[%T] %n: %v%$");

      core = spdlog::stdout_color_mt("ENGINE");
      core->set_level(spdlog::level::trace);

      client = spdlog::stdout_color_mt("APP");
      client->set_level(spdlog::level::trace);
    }
  };
  static Loggers s_Loggers;

  std::shared_ptr<spdlog::logger>& getCoreLogger() { return s_Loggers.core; }
  std::shared_ptr<spdlog::logger>& getClientLogger() { return s_Loggers.client; }
}