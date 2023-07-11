#include "ENpch.h"
#include "KeyEvent.h"

namespace Engine
{
  KeyPressEvent::KeyPressEvent(Key key, bool isRepeat)
    : m_Key(key), m_IsRepeat(isRepeat) {}

  EventType KeyPressEvent::type() const { return Type(); }
  EventType KeyPressEvent::Type() { return EventType::KeyPress; }
  EventCategory KeyPressEvent::categoryFlags() const { return EventCategory::Keyboard | EventCategory::Input; }
  const char* KeyPressEvent::name() const { return "KeyPress"; }
  std::string KeyPressEvent::toString() const
  {
    std::stringstream ss;
    ss << "KeyPressEvent: " << static_cast<keyID>(m_Key) << " (repeat = " << m_IsRepeat << ")";
    return ss.str();
  }

  Key KeyPressEvent::keyCode() const { return m_Key; }
  bool KeyPressEvent::isRepeat() const { return m_IsRepeat; }



  KeyReleaseEvent::KeyReleaseEvent(Key key)
    : m_Key(key) {}

  EventType KeyReleaseEvent::type() const { return Type(); }
  EventType KeyReleaseEvent::Type() { return EventType::KeyRelease; }
  EventCategory KeyReleaseEvent::categoryFlags() const { return EventCategory::Keyboard | EventCategory::Input; }
  const char* KeyReleaseEvent::name() const { return "KeyRelease"; }
  std::string KeyReleaseEvent::toString() const
  {
    std::stringstream ss;
    ss << "KeyReleaseEvent: " << static_cast<keyID>(m_Key);
    return ss.str();
  }

  Key KeyReleaseEvent::keyCode() const { return m_Key; }



  KeyTypeEvent::KeyTypeEvent(Key key)
    : m_Key(key) {}

  EventType KeyTypeEvent::type() const { return Type(); }
  EventType KeyTypeEvent::Type() { return EventType::KeyType; }
  EventCategory KeyTypeEvent::categoryFlags() const { return EventCategory::Keyboard | EventCategory::Input; }
  const char* KeyTypeEvent::name() const { return "KeyType"; }
  std::string KeyTypeEvent::toString() const
  {
    std::stringstream ss;
    ss << "KeyTypeEvent: " << static_cast<keyID>(m_Key);
    return ss.str();
  }

  Key KeyTypeEvent::keyCode() const { return m_Key; }
}