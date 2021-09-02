#pragma once

namespace Engine
{
  /*
    Events in Engine are currently blocking, meaning when an event occurs it
    immediately gets dispatched and must be dealt with right then and there.
    For the future, a better strategy might be to buffer events in an event
    bus and process them during the "event" part of the update stage.
  */

  enum class EventType : uint8_t
  {
    None = 0,
    WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMove,
    AppTick, AppUpdate, AppRender,
    KeyPress, KeyRelease, KeyType,
    MouseButtonPress, MouseButtonRelease, MouseMove, MouseScroll
  };

  enum class EventCategory : uint8_t
  {
    None = 0,
    Application  = bit(0),
    Input        = bit(1),
    Keyboard     = bit(2),
    Mouse        = bit(3),
    MouseButton  = bit(4)
  };
  ENABLE_BITMASK_OPERATORS(EventCategory);

  class Event
  {
  public:
    bool handled = false;

    virtual EventType getEventType() const = 0;
    virtual const char* getName() const = 0;
    virtual EventCategory getCategoryFlags() const = 0;
    virtual std::string toString() const { return getName(); }

    inline bool isInCategory(EventCategory category) { return (bool)(getCategoryFlags() & category); }
  };

  class EventDispatcher
  {
    template<typename T>
    using EventFn = std::function<bool(T&)>;

  public:
    EventDispatcher(Event& event)
      : m_Event(event) {}

    template<typename T>
    bool dispatch(EventFn<T> func)
    {
      if (m_Event.getEventType() == T::GetStaticType())
      {
        m_Event.handled = func(*(T*)&m_Event);
        return true;
      }
      return false;
    }

  private:
    Event& m_Event;
  };

  inline std::ostream& operator<<(std::ostream& os, const Event& e)
  {
    return os << e.toString();
  }
}