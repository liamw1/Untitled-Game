#pragma once
#include "Event.h"
#include "Engine/Core/KeyCodes.h"

namespace Engine
{
  class KeyPressEvent : public Event
  {
  public:
    KeyPressEvent(Key key, bool isRepeat = false);

    EventType type() const override;
    static EventType Type();
    EventCategory categoryFlags() const override;
    const char* name() const override;
    std::string toString() const override;

    Key keyCode() const;
    bool isRepeat() const;


  private:
    Key m_Key;
    bool m_IsRepeat;
  };

  class KeyReleaseEvent : public Event
  {
  public:
    KeyReleaseEvent(Key key);

    EventType type() const override;
    static EventType Type();
    EventCategory categoryFlags() const override;
    const char* name() const override;
    std::string toString() const override;

    Key keyCode() const;

  private:
    Key m_Key;
  };

  class KeyTypeEvent : public Event
  {
  public:
    KeyTypeEvent(Key key);

    EventType type() const override;
    static EventType Type();
    EventCategory categoryFlags() const override;
    const char* name() const override;
    std::string toString() const override;

    Key keyCode() const;

  private:
    Key m_Key;
  };
}