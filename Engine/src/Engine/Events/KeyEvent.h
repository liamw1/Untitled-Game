#pragma once
#include "EventCategory.h"
#include "Engine/Core/Input/KeyCodes.h"

namespace eng::event
{
  class KeyPress
  {
    input::Key m_Key;
    bool m_IsRepeat;

  public:
    KeyPress(input::Key key, bool isRepeat = false);

    EventCategory categoryFlags() const;
    const char* name() const;
    std::string toString() const;

    input::Key keyCode() const;
    bool isRepeat() const;
  };

  class KeyRelease
  {
    input::Key m_Key;

  public:
    KeyRelease(input::Key key);

    EventCategory categoryFlags() const;
    const char* name() const;
    std::string toString() const;

    input::Key keyCode() const;
  };

  class KeyType
  {
    input::Key m_Key;

  public:
    KeyType(input::Key key);

    EventCategory categoryFlags() const;
    const char* name() const;
    std::string toString() const;

    input::Key keyCode() const;
  };
}