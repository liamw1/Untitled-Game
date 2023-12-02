#pragma once
#include "EventCategory.h"
#include "Engine/Utilities/EnumUtilities.h"

namespace eng::event
{
  class AppRender
  {
  public:
    EnumBitMask<EventCategory> categoryFlags() const;
    const char* name() const;
  };

  class AppTick
  {
  public:
    EnumBitMask<EventCategory> categoryFlags() const;
    const char* name() const;
  };

  class AppUpdate
  {
  public:
    EnumBitMask<EventCategory> categoryFlags() const;
    const char* name() const;
  };

  class WindowClose
  {
  public:
    EnumBitMask<EventCategory> categoryFlags() const;
    const char* name() const;
  };

  class WindowResize
  {
    u32 m_Width;
    u32 m_Height;

  public:
    WindowResize(u32 width, u32 height);

    EnumBitMask<EventCategory> categoryFlags() const;
    const char* name() const;
    std::string toString() const;

    u32 width() const;
    u32 height() const;
  };
}