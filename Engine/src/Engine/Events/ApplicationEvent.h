#pragma once
#include "EventCategory.h"

namespace eng::event
{
  class AppRender
  {
  public:
    EventCategory categoryFlags() const;
    const char* name() const;
  };

  class AppTick
  {
  public:
    EventCategory categoryFlags() const;
    const char* name() const;
  };

  class AppUpdate
  {
  public:
    EventCategory categoryFlags() const;
    const char* name() const;
  };

  class WindowClose
  {
  public:
    EventCategory categoryFlags() const;
    const char* name() const;
  };

  class WindowResize
  {
  public:
    WindowResize(uint32_t width, uint32_t height);

    EventCategory categoryFlags() const;
    const char* name() const;
    std::string toString() const;

    uint32_t width() const;
    uint32_t height() const;

  private:
    uint32_t m_Width, m_Height;
  };
}