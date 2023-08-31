#pragma once

namespace Engine
{
  /*
    A contraint on classes that prevents copy construction and assignment.
    Meant to be privately inherited by non-copyable classes.
  */
  class NonCopyable
  {
  protected:
    NonCopyable() = default;
    ~NonCopyable() = default;

    NonCopyable(NonCopyable&& other) = default;
    NonCopyable& operator=(NonCopyable&& other) = default;

    NonCopyable(const NonCopyable& other) = delete;
    NonCopyable& operator=(const NonCopyable& other) = delete;
  };

  /*
    A contraint on classes that prevents move construction and assignment.
    Meant to be privately inherited by non-movable classes.
  */
  class NonMovable
  {
  protected:
    NonMovable() = default;
    ~NonMovable() = default;

    NonMovable(NonMovable&& other) = delete;
    NonMovable& operator=(NonMovable&& other) = delete;

    NonMovable(const NonMovable& other) = default;
    NonMovable& operator=(const NonMovable& other) = default;
  };
}