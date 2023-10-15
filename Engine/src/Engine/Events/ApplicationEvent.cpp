#include "ENpch.h"
#include "ApplicationEvent.h"

namespace eng::event
{
  WindowResize::WindowResize(uint32_t width, uint32_t height)
    : m_Width(width), m_Height(height) {}

  EventType WindowResize::type() const { return Type(); }
  EventType WindowResize::Type() { return EventType::WindowResize; }
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



  EventType WindowClose::type() const { return Type(); }
  EventType WindowClose::Type() { return EventType::WindowClose; }
  EventCategory WindowClose::categoryFlags() const { return EventCategory::Application; }
  const char* WindowClose::name() const { return "WindowClose"; }



  EventType AppTick::type() const { return Type(); }
  EventType AppTick::Type() { return EventType::AppTick; }
  EventCategory AppTick::categoryFlags() const { return EventCategory::Application; }
  const char* AppTick::name() const { return "AppTick"; }



  EventType AppUpdate::type() const { return Type(); }
  EventType AppUpdate::Type() { return EventType::AppUpdate; }
  EventCategory AppUpdate::categoryFlags() const { return EventCategory::Application; }
  const char* AppUpdate::name() const { return "AppUpdate"; }



  EventType AppRender::type() const { return Type(); }
  EventType AppRender::Type() { return EventType::AppRender; }
  EventCategory AppRender::categoryFlags() const { return EventCategory::Application; }
  const char* AppRender::name() const { return "AppRender"; }
}