#pragma once

namespace eng
{
  class Exception : public std::runtime_error
  {
  public:
    Exception(const std::string& message);
  };

  class CoreException : public std::runtime_error
  {
  public:
    CoreException(const std::string& message);
  };
}