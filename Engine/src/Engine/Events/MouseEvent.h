#pragma once
#include "Event.h"
#include "Engine/Core/Input/MouseButtonCodes.h"

namespace eng::event
{
  class MouseMove : public Event
  {
  public:
    MouseMove(float x, float y);

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

  class MouseScroll : public Event
  {
  public:
    MouseScroll(float xOffset, float yOffset);

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

  class MouseButtonPress : public Event
  {
  public:
    MouseButtonPress(input::Mouse button);

    EventType type() const override;
    static EventType Type();
    EventCategory categoryFlags() const override;
    const char* name() const override;
    std::string toString() const override;

  private:
    input::Mouse m_Button;
  };

  class MouseButtonRelease : public Event
  {
  public:
    MouseButtonRelease(input::Mouse button);

    EventType type() const override;
    static EventType Type();
    EventCategory categoryFlags() const override;
    const char* name() const override;
    std::string toString() const override;

  private:
    input::Mouse m_Button;
  };
}