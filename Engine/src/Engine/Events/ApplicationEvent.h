#pragma once
#include "ENpch.h"
#include "Event.h"

namespace Engine
{
  class ENGINE_API WindowResizeEvent : public Event
  {
  public:
    WindowResizeEvent(unsigned int width, unsigned int height)
      : m_Width(width), m_Height(height) {}

    static EventType GetStaticType() { return EventType::WindowResize; }
    virtual EventType getEventType() const override { return GetStaticType(); }
    virtual const char* getName() const override { return "WindowResize"; }
    int getCategoryFlags() const override { return EventCategoryApplication; }

    inline unsigned int getWidth() const { return m_Width; }
    inline unsigned int getHeight() const { return m_Height; }

    std::string toString() const override
    {
      std::stringstream ss;
      ss << "WindowResizeEvent: " << m_Width << ", " << m_Height;
      return ss.str();
    }

  private:
    unsigned int m_Width, m_Height;
  };

  class ENGINE_API WindowCloseEvent : public Event
  {
  public:
    WindowCloseEvent() {}

    static EventType GetStaticType() { return EventType::WindowClose; }
    virtual EventType getEventType() const override { return GetStaticType(); }
    virtual const char* getName() const override { return "WindowClose"; }
    int getCategoryFlags() const override { return EventCategoryApplication; }
  };

  class ENGINE_API AppTickEvent : public Event
  {
  public:
    AppTickEvent() {}

    static EventType GetStaticType() { return EventType::AppTick; }
    virtual EventType getEventType() const override { return GetStaticType(); }
    virtual const char* getName() const override { return "AppTick"; }
    int getCategoryFlags() const override { return EventCategoryApplication; }
  };

  class ENGINE_API AppUpdateEvent : public Event
  {
  public:
    AppUpdateEvent() {}

    static EventType GetStaticType() { return EventType::AppUpdate; }
    virtual EventType getEventType() const override { return GetStaticType(); }
    virtual const char* getName() const override { return "AppUpdate"; }
    int getCategoryFlags() const override { return EventCategoryApplication; }
  };

  class ENGINE_API AppRenderEvent : public Event
  {
  public:
    AppRenderEvent() {}

    static EventType GetStaticType() { return EventType::AppRender; }
    virtual EventType getEventType() const override { return GetStaticType(); }
    virtual const char* getName() const override { return "AppRender"; }
    int getCategoryFlags() const override { return EventCategoryApplication; }
  };
}