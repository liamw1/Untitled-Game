#pragma once
#include "Event.h"

namespace eng::event
{
  class WindowResize : public Event
  {
  public:
    WindowResize(uint32_t width, uint32_t height);

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

  class WindowClose : public Event
  {
  public:
    EventType type() const override;
    static EventType Type();
    EventCategory categoryFlags() const override;
    const char* name() const override;
  };

  class AppTick : public Event
  {
  public:
    EventType type() const override;
    static EventType Type();
    EventCategory categoryFlags() const override;
    const char* name() const override;
  };

  class AppUpdate : public Event
  {
  public:
    EventType type() const override;
    static EventType Type();
    EventCategory categoryFlags() const override;
    const char* name() const override;
  };

  class AppRender : public Event
  {
  public:
    EventType type() const override;
    static EventType Type();
    EventCategory categoryFlags() const override;
    const char* name() const override;
  };
}