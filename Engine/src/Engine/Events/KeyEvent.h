#pragma once
#include "EventCategory.h"
#include "Engine/Core/Input/KeyCodes.h"

namespace eng::event
{
  class KeyPress
  {
  public:
    KeyPress(input::Key key, bool isRepeat = false);

    EventCategory categoryFlags() const;
    const char* name() const;
    std::string toString() const;

    input::Key keyCode() const;
    bool isRepeat() const;

  private:
    input::Key m_Key;
    bool m_IsRepeat;
  };

  class KeyRelease
  {
  public:
    KeyRelease(input::Key key);

    EventCategory categoryFlags() const;
    const char* name() const;
    std::string toString() const;

    input::Key keyCode() const;

  private:
    input::Key m_Key;
  };

  class KeyType
  {
  public:
    KeyType(input::Key key);

    EventCategory categoryFlags() const;
    const char* name() const;
    std::string toString() const;

    input::Key keyCode() const;

  private:
    input::Key m_Key;
  };
}