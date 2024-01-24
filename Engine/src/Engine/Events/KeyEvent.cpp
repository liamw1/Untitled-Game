#include "ENpch.h"
#include "KeyEvent.h"

namespace eng::event
{
  KeyPress::KeyPress(input::Key key, bool isRepeat)
    : m_Key(key), m_IsRepeat(isRepeat) {}

  EnumBitMask<EventCategory> KeyPress::categoryFlags() const { return { EventCategory::Keyboard, EventCategory::Input }; }
  std::string_view KeyPress::name() const { return "KeyPress"; }
  std::string KeyPress::toString() const
  {
    std::stringstream ss;
    ss << "KeyPress: " << toUnderlying(m_Key) << " (repeat = " << m_IsRepeat << ")";
    return ss.str();
  }

  input::Key KeyPress::keyCode() const { return m_Key; }
  bool KeyPress::isRepeat() const { return m_IsRepeat; }



  KeyRelease::KeyRelease(input::Key key)
    : m_Key(key) {}

  EnumBitMask<EventCategory> KeyRelease::categoryFlags() const { return { EventCategory::Keyboard, EventCategory::Input }; }
  std::string_view KeyRelease::name() const { return "KeyRelease"; }
  std::string KeyRelease::toString() const
  {
    std::stringstream ss;
    ss << "KeyRelease: " << toUnderlying(m_Key);
    return ss.str();
  }

  input::Key KeyRelease::keyCode() const { return m_Key; }



  KeyType::KeyType(input::Key key)
    : m_Key(key) {}

  EnumBitMask<EventCategory> KeyType::categoryFlags() const { return { EventCategory::Keyboard, EventCategory::Input }; }
  std::string_view KeyType::name() const { return "KeyType"; }
  std::string KeyType::toString() const
  {
    std::stringstream ss;
    ss << "KeyType: " << toUnderlying(m_Key);
    return ss.str();
  }

  input::Key KeyType::keyCode() const { return m_Key; }
}