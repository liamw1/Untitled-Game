#include "ENpch.h"
#include "ApplicationEvent.h"

namespace eng::event
{
  EnumBitMask<EventCategory> AppTick::categoryFlags() const { return EventCategory::Application; }
  std::string_view AppTick::name() const { return "AppTick"; }



  EnumBitMask<EventCategory> AppUpdate::categoryFlags() const { return EventCategory::Application; }
  std::string_view AppUpdate::name() const { return "AppUpdate"; }



  EnumBitMask<EventCategory> AppRender::categoryFlags() const { return EventCategory::Application; }
  std::string_view AppRender::name() const { return "AppRender"; }



  EnumBitMask<EventCategory> WindowClose::categoryFlags() const { return EventCategory::Application; }
  std::string_view WindowClose::name() const { return "WindowClose"; }



  WindowResize::WindowResize(u32 width, u32 height)
    : m_Width(width), m_Height(height) {}

  EnumBitMask<EventCategory> WindowResize::categoryFlags() const { return EventCategory::Application; }
  std::string_view WindowResize::name() const { return "WindowResized"; }

  std::string WindowResize::toString() const
  {
    std::stringstream ss;
    ss << "WindowResizedEvent: " << m_Width << ", " << m_Height;
    return ss.str();
  }

  u32 WindowResize::width() const { return m_Width; }
  u32 WindowResize::height() const { return m_Height; }
}