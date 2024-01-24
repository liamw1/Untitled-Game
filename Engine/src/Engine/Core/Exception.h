#pragma once

namespace eng
{
  class Exception : public std::runtime_error
  {
  public:
    Exception(std::string_view message);
  };

  class CoreException : public std::runtime_error
  {
  public:
    CoreException(std::string_view message);
  };
}