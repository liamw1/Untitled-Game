#include "ENpch.h"
#include "ApplicationEvent.h"

namespace Engine
{
  WindowResizeEvent::WindowResizeEvent(uint32_t width, uint32_t height)
    : m_Width(width), m_Height(height) {}

  EventType WindowResizeEvent::type() const { return Type(); }
  EventType WindowResizeEvent::Type() { return EventType::WindowResize; }
  EventCategory WindowResizeEvent::categoryFlags() const { return EventCategory::Application; }
  const char* WindowResizeEvent::name() const { return "WindowResized"; }

  std::string WindowResizeEvent::toString() const
  {
    std::stringstream ss;
    ss << "WindowResizedEvent: " << m_Width << ", " << m_Height;
    return ss.str();
  }

  uint32_t WindowResizeEvent::width() const { return m_Width; }
  uint32_t WindowResizeEvent::height() const { return m_Height; }



  EventType WindowCloseEvent::type() const { return Type(); }
  EventType WindowCloseEvent::Type() { return EventType::WindowClose; }
  EventCategory WindowCloseEvent::categoryFlags() const { return EventCategory::Application; }
  const char* WindowCloseEvent::name() const { return "WindowClose"; }



  EventType AppTickEvent::type() const { return Type(); }
  EventType AppTickEvent::Type() { return EventType::AppTick; }
  EventCategory AppTickEvent::categoryFlags() const { return EventCategory::Application; }
  const char* AppTickEvent::name() const { return "AppTick"; }



  EventType AppUpdateEvent::type() const { return Type(); }
  EventType AppUpdateEvent::Type() { return EventType::AppUpdate; }
  EventCategory AppUpdateEvent::categoryFlags() const { return EventCategory::Application; }
  const char* AppUpdateEvent::name() const { return "AppUpdate"; }



  EventType AppRenderEvent::type() const { return Type(); }
  EventType AppRenderEvent::Type() { return EventType::AppRender; }
  EventCategory AppRenderEvent::categoryFlags() const { return EventCategory::Application; }
  const char* AppRenderEvent::name() const { return "AppRender"; }
}