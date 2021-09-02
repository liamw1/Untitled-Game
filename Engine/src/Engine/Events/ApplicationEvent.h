#pragma once
#include "Event.h"

namespace Engine
{
  class WindowResizeEvent : public Event
  {
  public:
    WindowResizeEvent(unsigned int width, unsigned int height)
      : m_Width(width), m_Height(height) {}

    static EventType GetStaticType() { return EventType::WindowResize; }
    EventType getEventType() const override { return GetStaticType(); }
    const char* getName() const override { return "WindowResized"; }
    EventCategory getCategoryFlags() const override { return EventCategory::Application; }

    inline unsigned int getWidth() const { return m_Width; }
    inline unsigned int getHeight() const { return m_Height; }

    std::string toString() const override
    {
      std::stringstream ss;
      ss << "WindowResizedEvent: " << m_Width << ", " << m_Height;
      return ss.str();
    }

  private:
    unsigned int m_Width, m_Height;
  };

  class WindowCloseEvent : public Event
  {
  public:
    WindowCloseEvent() {}

    static EventType GetStaticType() { return EventType::WindowClose; }
    EventType getEventType() const override { return GetStaticType(); }
    const char* getName() const override { return "WindowClose"; }
    EventCategory getCategoryFlags() const override { return EventCategory::Application; }
  };

  class AppTickEvent : public Event
  {
  public:
    AppTickEvent() {}

    static EventType GetStaticType() { return EventType::AppTick; }
    EventType getEventType() const override { return GetStaticType(); }
    const char* getName() const override { return "AppTick"; }
    EventCategory getCategoryFlags() const override { return EventCategory::Application; }
  };

  class AppUpdateEvent : public Event
  {
  public:
    AppUpdateEvent() {}

    static EventType GetStaticType() { return EventType::AppUpdate; }
    EventType getEventType() const override { return GetStaticType(); }
    const char* getName() const override { return "AppUpdate"; }
    EventCategory getCategoryFlags() const override { return EventCategory::Application; }
  };

  class AppRenderEvent : public Event
  {
  public:
    AppRenderEvent() {}

    static EventType GetStaticType() { return EventType::AppRender; }
    EventType getEventType() const override { return GetStaticType(); }
    const char* getName() const override { return "AppRender"; }
    EventCategory getCategoryFlags() const override { return EventCategory::Application; }
  };
}