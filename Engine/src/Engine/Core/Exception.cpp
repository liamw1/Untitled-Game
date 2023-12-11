#include "ENpch.h"
#include "Exception.h"
#include "Engine/Debug/Assert.h"

namespace eng
{
  Exception::Exception(const std::string& message)
    : std::runtime_error(message) { ENG_TERMINATE(message); }

  CoreException::CoreException(const std::string& message)
    : std::runtime_error(message) { ENG_CORE_TERMINATE(message); }
}
