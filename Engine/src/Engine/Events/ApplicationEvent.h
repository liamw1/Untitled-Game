#pragma once
#include "Event.h"

namespace Engine
{
  class WindowResizeEvent : public Event
  {
  public:
    WindowResizeEvent(uint32_t width, uint32_t height);

    EventType type() const override;
    static EventType Type();
    EventCategory categoryFlags() const override;
    const char* name() const override;
    std::string toString() const override;

    uint32_t width() const;
    uint32_t height() const;

  private:
    uint32_t m_Width, m_Height;
  };

  class WindowCloseEvent : public Event
  {
  public:
    EventType type() const override;
    static EventType Type();
    EventCategory categoryFlags() const override;
    const char* name() const override;
  };

  class AppTickEvent : public Event
  {
  public:
    EventType type() const override;
    static EventType Type();
    EventCategory categoryFlags() const override;
    const char* name() const override;
  };

  class AppUpdateEvent : public Event
  {
  public:
    EventType type() const override;
    static EventType Type();
    EventCategory categoryFlags() const override;
    const char* name() const override;
  };

  class AppRenderEvent : public Event
  {
  public:
    EventType type() const override;
    static EventType Type();
    EventCategory categoryFlags() const override;
    const char* name() const override;
  };
}