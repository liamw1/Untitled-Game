#pragma once
#include "ENpch.h"
#include "Event.h"

namespace Engine
{
  class ENGINE_API KeyEvent : public Event
  {
  public:
    int getCategoryFlags() const override { return EventCategoryKeyboard | EventCategoryInput; }

    inline int getKeyCode() const { return m_KeyCode; }

  protected:
    KeyEvent(int keycode)
      : m_KeyCode(keycode) {}

    int m_KeyCode;
  };

  class ENGINE_API KeyPressedEvent : public KeyEvent
  {
  public:
    KeyPressedEvent(int keycode, int repeatCount)
      : KeyEvent(keycode), m_RepeatCount(repeatCount) {}

    static EventType GetStaticType() { return EventType::KeyPressed; }
    virtual EventType getEventType() const override { return GetStaticType(); }
    virtual const char* getName() const override { return "KeyPressed"; }

    inline int getRepeatCount() const { return m_RepeatCount; }

    std::string toString() const override
    {
      std::stringstream ss;
      ss << "KeyPressedEvent: " << m_KeyCode << " (" << m_RepeatCount << " repeats)";
      return ss.str();
    }

  private:
    int m_RepeatCount;
  };

  class ENGINE_API KeyReleasedEvent : public KeyEvent
  {
  public:
    KeyReleasedEvent(int keyCode)
      : KeyEvent(keyCode) {}

    static EventType GetStaticType() { return EventType::KeyReleased; }
    virtual EventType getEventType() const override { return GetStaticType(); }
    virtual const char* getName() const override { return "KeyReleased"; }

    std::string toString() const override
    {
      std::stringstream ss;
      ss << "KeyReleasedEvent: " << m_KeyCode;
      return ss.str();
    }
  };
}