#pragma once
#include "Event.h"
#include "Engine/Core/MouseButtonCodes.h"

namespace Engine
{
  class ENGINE_API MouseMoveEvent : public Event
  {
  public:
    MouseMoveEvent(float x, float y)
      : m_MouseX(x), m_MouseY(y) {}

    static EventType GetStaticType() { return EventType::MouseMove; }
    EventType getEventType() const override { return GetStaticType(); }
    const char* getName() const override { return "MouseMove"; }
    int getCategoryFlags() const override { return EventCategoryMouse | EventCategoryInput; }

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

  class ENGINE_API MouseScrollEvent : public Event
  {
  public:
    MouseScrollEvent(float xOffset, float yOffset)
      : m_XOffset(xOffset), m_YOffset(yOffset) {}

    static EventType GetStaticType() { return EventType::MouseScroll; }
    EventType getEventType() const override { return GetStaticType(); }
    const char* getName() const override { return "MouseScroll"; }
    int getCategoryFlags() const override { return EventCategoryMouse | EventCategoryInput; }

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

  class ENGINE_API MouseButtonEvent : public Event
  {
  public:
    int getCategoryFlags() const override { return EventCategoryMouse | EventCategoryInput; }

    inline MouseButton getMouseButton() const { return m_Button; }

  protected:
    MouseButtonEvent(MouseButton button)
      : m_Button(button) {}

    MouseButton m_Button;
  };

  class ENGINE_API MouseButtonPressEvent : public MouseButtonEvent
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

  class ENGINE_API MouseButtonReleaseEvent : public MouseButtonEvent
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