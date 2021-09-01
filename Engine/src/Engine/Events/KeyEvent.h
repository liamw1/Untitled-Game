#pragma once
#include "ENpch.h"
#include "Event.h"
#include "Engine/Core/KeyCodes.h"

namespace Engine
{
  class ENGINE_API KeyEvent : public Event
  {
  public:
    int getCategoryFlags() const override { return EventCategoryKeyboard | EventCategoryInput; }

    inline Key getKeyCode() const { return m_KeyCode; }

  protected:
    KeyEvent(Key keycode)
      : m_KeyCode(keycode) {}

    Key m_KeyCode;
  };

  class ENGINE_API KeyPressEvent : public KeyEvent
  {
  public:
    KeyPressEvent(Key keycode, int repeatCount)
      : KeyEvent(keycode), m_RepeatCount(repeatCount) {}

    static EventType GetStaticType() { return EventType::KeyPress; }
    EventType getEventType() const override { return GetStaticType(); }
    const char* getName() const override { return "KeyPress"; }

    inline int getRepeatCount() const { return m_RepeatCount; }

    std::string toString() const override
    {
      std::stringstream ss;
      ss << "KeyPressEvent: " << (keyCode)m_KeyCode << " (" << m_RepeatCount << " repeats)";
      return ss.str();
    }

  private:
    int m_RepeatCount;
  };

  class ENGINE_API KeyReleaseEvent : public KeyEvent
  {
  public:
    KeyReleaseEvent(Key keyCode)
      : KeyEvent(keyCode) {}

    static EventType GetStaticType() { return EventType::KeyRelease; }
    EventType getEventType() const override { return GetStaticType(); }
    const char* getName() const override { return "KeyRelease"; }

    std::string toString() const override
    {
      std::stringstream ss;
      ss << "KeyReleaseEvent: " << (keyCode)m_KeyCode;
      return ss.str();
    }
  };

  class ENGINE_API KeyTypeEvent : public KeyEvent
  {
  public:
    KeyTypeEvent(Key keycode)
      : KeyEvent(keycode) {}

    static EventType GetStaticType() { return EventType::KeyType; }
    EventType getEventType() const override { return GetStaticType(); }
    const char* getName() const override { return "KeyType"; }

    std::string toString() const override
    {
      std::stringstream ss;
      ss << "KeyTypeEvent: " << (keyCode)m_KeyCode;
      return ss.str();
    }
  };
}