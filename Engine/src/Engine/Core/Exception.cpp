#include "ENpch.h"
#include "Exception.h"
#include "Engine/Debug/Assert.h"

namespace eng
{
  Exception::Exception(std::string_view message)
    : std::runtime_error(std::string(message)) { ENG_TERMINATE(message); }

  CoreException::CoreException(std::string_view message)
    : std::runtime_error(std::string(message)) { ENG_CORE_TERMINATE(message); }
}
