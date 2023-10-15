#pragma once
#include "Event.h"
#include "Engine/Core/Input/KeyCodes.h"

namespace eng::event
{
  class KeyPress : public Event
  {
  public:
    KeyPress(input::Key key, bool isRepeat = false);

    EventType type() const override;
    static EventType Type();
    EventCategory categoryFlags() const override;
    const char* name() const override;
    std::string toString() const override;

    input::Key keyCode() const;
    bool isRepeat() const;


  private:
    input::Key m_Key;
    bool m_IsRepeat;
  };

  class KeyRelease : public Event
  {
  public:
    KeyRelease(input::Key key);

    EventType type() const override;
    static EventType Type();
    EventCategory categoryFlags() const override;
    const char* name() const override;
    std::string toString() const override;

    input::Key keyCode() const;

  private:
    input::Key m_Key;
  };

  class KeyType : public Event
  {
  public:
    KeyType(input::Key key);

    EventType type() const override;
    static EventType Type();
    EventCategory categoryFlags() const override;
    const char* name() const override;
    std::string toString() const override;

    input::Key keyCode() const;

  private:
    input::Key m_Key;
  };
}