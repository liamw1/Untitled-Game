#include "ENpch.h"
#include "KeyEvent.h"

namespace eng::event
{
  KeyPress::KeyPress(input::Key key, bool isRepeat)
    : m_Key(key), m_IsRepeat(isRepeat) {}

  EventType KeyPress::type() const { return Type(); }
  EventType KeyPress::Type() { return EventType::KeyPress; }
  EventCategory KeyPress::categoryFlags() const { return EventCategory::Keyboard | EventCategory::Input; }
  const char* KeyPress::name() const { return "KeyPress"; }
  std::string KeyPress::toString() const
  {
    std::stringstream ss;
    ss << "KeyPress: " << static_cast<input::keyID>(m_Key) << " (repeat = " << m_IsRepeat << ")";
    return ss.str();
  }

  input::Key KeyPress::keyCode() const { return m_Key; }
  bool KeyPress::isRepeat() const { return m_IsRepeat; }



  KeyRelease::KeyRelease(input::Key key)
    : m_Key(key) {}

  EventType KeyRelease::type() const { return Type(); }
  EventType KeyRelease::Type() { return EventType::KeyRelease; }
  EventCategory KeyRelease::categoryFlags() const { return EventCategory::Keyboard | EventCategory::Input; }
  const char* KeyRelease::name() const { return "KeyRelease"; }
  std::string KeyRelease::toString() const
  {
    std::stringstream ss;
    ss << "KeyRelease: " << static_cast<input::keyID>(m_Key);
    return ss.str();
  }

  input::Key KeyRelease::keyCode() const { return m_Key; }



  KeyType::KeyType(input::Key key)
    : m_Key(key) {}

  EventType KeyType::type() const { return Type(); }
  EventType KeyType::Type() { return EventType::KeyType; }
  EventCategory KeyType::categoryFlags() const { return EventCategory::Keyboard | EventCategory::Input; }
  const char* KeyType::name() const { return "KeyType"; }
  std::string KeyType::toString() const
  {
    std::stringstream ss;
    ss << "KeyType: " << static_cast<input::keyID>(m_Key);
    return ss.str();
  }

  input::Key KeyType::keyCode() const { return m_Key; }
}