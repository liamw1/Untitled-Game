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



  WindowResize::WindowResize(uint32_t width, uint32_t height)
    : m_Width(width), m_Height(height) {}

  EventCategory WindowResize::categoryFlags() const { return EventCategory::Application; }
  const char* WindowResize::name() const { return "WindowResized"; }

  std::string WindowResize::toString() const
  {
    std::stringstream ss;
    ss << "WindowResizedEvent: " << m_Width << ", " << m_Height;
    return ss.str();
  }

  uint32_t WindowResize::width() const { return m_Width; }
  uint32_t WindowResize::height() const { return m_Height; }
}