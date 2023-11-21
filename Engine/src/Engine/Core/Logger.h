#pragma once
#include <codeanalysis\warnings.h> // Disable intellisense warnings
#pragma warning(push, 0)
#pragma warning(disable : ALL_CODE_ANALYSIS_WARNINGS)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#pragma warning(pop)

namespace eng::log
{
  std::shared_ptr<spdlog::logger>& getCoreLogger();
  std::shared_ptr<spdlog::logger>& getClientLogger();
}

// Core log macros
#define ENG_CORE_TRACE(...)   ::eng::log::getCoreLogger()->trace(__VA_ARGS__)
#define ENG_CORE_INFO(...)    ::eng::log::getCoreLogger()->info(__VA_ARGS__)
#define ENG_CORE_WARN(...)    ::eng::log::getCoreLogger()->warn(__VA_ARGS__)
#define ENG_CORE_ERROR(...)   ::eng::log::getCoreLogger()->error(__VA_ARGS__)
#define ENG_CORE_FATAL(...)   ::eng::log::getCoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define ENG_TRACE(...)        ::eng::log::getClientLogger()->trace(__VA_ARGS__)
#define ENG_INFO(...)         ::eng::log::getClientLogger()->info(__VA_ARGS__)
#define ENG_WARN(...)         ::eng::log::getClientLogger()->warn(__VA_ARGS__)
#define ENG_ERROR(...)        ::eng::log::getClientLogger()->error(__VA_ARGS__)
#define ENG_FATAL(...)        ::eng::log::getClientLogger()->critical(__VA_ARGS__)
