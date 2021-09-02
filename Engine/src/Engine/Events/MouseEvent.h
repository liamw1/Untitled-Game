#pragma once
#include "Event.h"
#include "Engine/Core/MouseButtonCodes.h"

namespace Engine
{
  class MouseMoveEvent : public Event
  {
  public:
    MouseMoveEvent(float x, float y)
      : m_MouseX(x), m_MouseY(y) {}

    static EventType GetStaticType() { return EventType::MouseMove; }
    EventType getEventType() const override { return GetStaticType(); }
    const char* getName() const override { return "MouseMove"; }
    EventCategory getCategoryFlags() const override { return EventCategory::Mouse | EventCategory::Input; }

    inline float getX() const { return m_MouseX; }
    inline float getY() const { return m_MouseY; }

    std::string toString() const override
    {
      std::stringstream ss;
      ss << "MouseMoveEvent: " << m_MouseX << ", " << m_MouseY;
      return ss.str();
    }

  private:
    float m_MouseX, m_MouseY;
  };

  class MouseScrollEvent : public Event
  {
  public:
    MouseScrollEvent(float xOffset, float yOffset)
      : m_XOffset(xOffset), m_YOffset(yOffset) {}

    static EventType GetStaticType() { return EventType::MouseScroll; }
    EventType getEventType() const override { return GetStaticType(); }
    const char* getName() const override { return "MouseScroll"; }
    EventCategory getCategoryFlags() const override { return EventCategory::Mouse | EventCategory::Input; }

    inline float getXOffset() const { return m_XOffset; }
    inline float getYOffset() const { return m_YOffset; }

    std::string toString() const override
    {
      std::stringstream ss;
      ss << "MouseScrollEvent: " << getXOffset() << ", " << getYOffset();
      return ss.str();
    }

  private:
    float m_XOffset, m_YOffset;
  };

  class MouseButtonEvent : public Event
  {
  public:
    EventCategory getCategoryFlags() const override { return EventCategory::Mouse | EventCategory::Input; }

    inline MouseButton getMouseButton() const { return m_Button; }

  protected:
    MouseButtonEvent(MouseButton button)
      : m_Button(button) {}

    MouseButton m_Button;
  };

  class MouseButtonPressEvent : public MouseButtonEvent
  {
  public:
    MouseButtonPressEvent(MouseButton button)
      : MouseButtonEvent(button) {}

    static EventType GetStaticType() { return EventType::MouseButtonPress; }
    EventType getEventType() const override { return GetStaticType(); }
    const char* getName() const override { return "MouseButtonPress"; }

    std::string toString() const override
    {
      std::stringstream ss;
      ss << "MouseButtonPressEvent: " << (mouseCode)m_Button;
      return ss.str();
    }
  };

  class MouseButtonReleaseEvent : public MouseButtonEvent
  {
  public:
    MouseButtonReleaseEvent(MouseButton button)
      : MouseButtonEvent(button) {}

    static EventType GetStaticType() { return EventType::MouseButtonRelease; }
    EventType getEventType() const override { return GetStaticType(); }
    const char* getName() const override { return "MouseButtonRelease"; }

    std::string toString() const override
    {
      std::stringstream ss;
      ss << "MouseButtonReleaseEvent: " << (mouseCode)m_Button;
      return ss.str();
    }
  };
}