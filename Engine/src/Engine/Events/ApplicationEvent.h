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
    WindowResize(u32 width, u32 height);

    EventCategory categoryFlags() const;
    const char* name() const;
    std::string toString() const;

    u32 width() const;
    u32 height() const;

  private:
    u32 m_Width;
    u32 m_Height;
  };
}