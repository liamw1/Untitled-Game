#include "ENpch.h"
#include "ApplicationEvent.h"

namespace eng::event
{
  EventCategory AppTick::categoryFlags() const { return EventCategory::Application; }
  const char* AppTick::name() const { return "AppTick"; }



  EventCategory AppUpdate::categoryFlags() const { return EventCategory::Application; }
  const char* AppUpdate::name() const { return "AppUpdate"; }



  EventCategory AppRender::categoryFlags() const { return EventCategory::Application; }
  const char* AppRender::name() const { return "AppRender"; }



  EventCategory WindowClose::categoryFlags() const { return EventCategory::Application; }
  const char* WindowClose::name() const { return "WindowClose"; }



  WindowResize::WindowResize(u32 width, u32 height)
    : m_Width(width), m_Height(height) {}

  EventCategory WindowResize::categoryFlags() const { return EventCategory::Application; }
  const char* WindowResize::name() const { return "WindowResized"; }

  std::string WindowResize::toString() const
  {
    std::stringstream ss;
    ss << "WindowResizedEvent: " << m_Width << ", " << m_Height;
    return ss.str();
  }

  u32 WindowResize::width() const { return m_Width; }
  u32 WindowResize::height() const { return m_Height; }
}