#pragma once
#include "ENpch.h"

namespace Engine
{
  /*
    Events in Engine are currently blocking, meaning when an event occurs it
    immediately gets dispatched and must be dealt with right then and there.
    For the future, a better strategy might be to buffer events in an event
    bus and process them during the "event" part of the update stage.
  */

  inline static constexpr unsigned char bit(unsigned char n) { return 1 << n; }

  enum class EventType
  {
    None = 0,
    WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMove,
    AppTick, AppUpdate, AppRender,
    KeyPress, KeyRelease, KeyType,
    MouseButtonPress, MouseButtonRelease, MouseMove, MouseScroll
  };

  enum EventCategory
  {
    None = 0,
    EventCategoryApplication  = bit(0),
    EventCategoryInput        = bit(1),
    EventCategoryKeyboard     = bit(2),
    EventCategoryMouse        = bit(3),
    EventCategoryMouseButton  = bit(4)
  };

  class ENGINE_API Event
  {
  public:
    bool handled = false;

    virtual EventType getEventType() const = 0;
    virtual const char* getName() const = 0;
    virtual int getCategoryFlags() const = 0;
    virtual std::string toString() const { return getName(); }

    inline bool isInCategory(EventCategory category) { return getCategoryFlags() & category; }
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