#pragma once
#include "ENpch.h"
#include "Event.h"

namespace Engine
{
  class ENGINE_API MouseMovedEvent : public Event
  {
  public:
    MouseMovedEvent(float x, float y)
      : m_MouseX(x), m_MouseY(y) {}

    static EventType GetStaticType() { return EventType::MouseMoved; }
    virtual EventType getEventType() const override { return GetStaticType(); }
    virtual const char* getName() const override { return "MouseMoved"; }
    int getCategoryFlags() const override { return EventCategoryMouse | EventCategoryInput; }

    inline float getX() const { return m_MouseX; }
    inline float getY() const { return m_MouseY; }

    std::string toString() const override
    {
      std::stringstream ss;
      ss << "MouseMovedEvent: " << m_MouseX << ", " << m_MouseY;
      return ss.str();
    }

  private:
    float m_MouseX, m_MouseY;
  };

  class ENGINE_API MouseScrolledEvent : public Event
  {
  public:
    MouseScrolledEvent(float xOffset, float yOffset)
      : m_XOffset(xOffset), m_YOffset(yOffset) {}

    static EventType GetStaticType() { return EventType::MouseMoved; }
    virtual EventType getEventType() const override { return GetStaticType(); }
    virtual const char* getName() const override { return "MouseMoved"; }
    int getCategoryFlags() const override { return EventCategoryMouse | EventCategoryInput; }

    inline float getXOffset() const { return m_XOffset; }
    inline float getYOffset() const { return m_YOffset; }

    std::string toString() const override
    {
      std::stringstream ss;
      ss << "MouseScrolledEvent: " << getXOffset() << ", " << getYOffset();
      return ss.str();
    }

  private:
    float m_XOffset, m_YOffset;
  };

  class ENGINE_API MouseButtonEvent : public Event
  {
  public:
    int getCategoryFlags() const override { return EventCategoryMouse | EventCategoryInput; }

    inline int getMouseButton() const { return m_Button; }

  protected:
    MouseButtonEvent(int button)
      : m_Button(button) {}

    int m_Button;
  };

  class ENGINE_API MouseButtonPressedEvent : public MouseButtonEvent
  {
  public:
    MouseButtonPressedEvent(int button)
      : MouseButtonEvent(button) {}

    static EventType GetStaticType() { return EventType::MouseButtonPressed; }
    virtual EventType getEventType() const override { return GetStaticType(); }
    virtual const char* getName() const override { return "MouseButtonPressed"; }

    std::string toString() const override
    {
      std::stringstream ss;
      ss << "MouseButtonPressedEvent: " << m_Button;
      return ss.str();
    }
  };

  class ENGINE_API MouseButtonReleasedEvent : public MouseButtonEvent
  {
  public:
    MouseButtonReleasedEvent(int button)
      : MouseButtonEvent(button) {}

    static EventType GetStaticType() { return EventType::MouseButtonReleased; }
    virtual EventType getEventType() const override { return GetStaticType(); }
    virtual const char* getName() const override { return "MouseButtonReleased"; }

    std::string toString() const override
    {
      std::stringstream ss;
      ss << "MouseButtonReleasedEvent: " << m_Button;
      return ss.str();
    }
  };
}