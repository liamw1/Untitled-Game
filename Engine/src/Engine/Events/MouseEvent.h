#pragma once
#include "Event.h"
#include "Engine/Core/MouseButtonCodes.h"

namespace Engine
{
  class MouseMoveEvent : public Event
  {
  public:
    MouseMoveEvent(float x, float y);

    EventType type() const override;
    static EventType Type();
    EventCategory categoryFlags() const override;
    const char* name() const override;
    std::string toString() const override;

    float x() const;
    float y() const;

  private:
    float m_MouseX, m_MouseY;
  };

  class MouseScrollEvent : public Event
  {
  public:
    MouseScrollEvent(float xOffset, float yOffset);

    EventType type() const override;
    static EventType Type();
    EventCategory categoryFlags() const override;
    const char* name() const override;
    std::string toString() const override;

    float xOffset() const;
    float yOffset() const;

  private:
    float m_XOffset, m_YOffset;
  };

  class MouseButtonPressEvent : public Event
  {
  public:
    MouseButtonPressEvent(Mouse button);

    EventType type() const override;
    static EventType Type();
    EventCategory categoryFlags() const override;
    const char* name() const override;
    std::string toString() const override;

  private:
    Mouse m_Button;
  };

  class MouseButtonReleaseEvent : public Event
  {
  public:
    MouseButtonReleaseEvent(Mouse button);

    EventType type() const override;
    static EventType Type();
    EventCategory categoryFlags() const override;
    const char* name() const override;
    std::string toString() const override;

  private:
    Mouse m_Button;
  };
}