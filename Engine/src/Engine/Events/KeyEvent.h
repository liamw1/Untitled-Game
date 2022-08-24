#pragma once
#include "Event.h"
#include "Engine/Core/KeyCodes.h"

namespace Engine
{
  class KeyEvent : public Event
  {
  public:
    EventCategory getCategoryFlags() const override { return EventCategory::Keyboard | EventCategory::Input; }

    Key getKeyCode() const { return m_Key; }

  protected:
    KeyEvent(Key keycode)
      : m_Key(keycode) {}

    Key m_Key;
  };

  class KeyPressEvent : public KeyEvent
  {
  public:
    KeyPressEvent(Key keycode, bool isRepeat = false)
      : KeyEvent(keycode), m_IsRepeat(isRepeat) {}

    static EventType GetStaticType() { return EventType::KeyPress; }
    EventType getEventType() const override { return GetStaticType(); }
    const char* getName() const override { return "KeyPress"; }

    bool isRepeat() const { return m_IsRepeat; }

    std::string toString() const override
    {
      std::stringstream ss;
      ss << "KeyPressEvent: " << static_cast<keyCode>(m_Key) << " (repeat = " << m_IsRepeat << ")";
      return ss.str();
    }

  private:
    bool m_IsRepeat;
  };

  class KeyReleaseEvent : public KeyEvent
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
      ss << "KeyReleaseEvent: " << static_cast<keyCode>(m_Key);
      return ss.str();
    }
  };

  class KeyTypeEvent : public KeyEvent
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
      ss << "KeyTypeEvent: " << static_cast<keyCode>(m_Key);
      return ss.str();
    }
  };
}