#pragma once
#include "Engine/Utilities/BitUtilities.h"
#include "Engine/Utilities/EnumUtilities.h"

namespace Engine
{
  /*
    NOTE:
    Events in Engine are currently blocking, meaning when an event occurs it
    immediately gets dispatched and must be dealt with right then and there.
    For the future, a better strategy might be to buffer events in an event
    bus and process them during the "event" part of the update stage.
  */

  enum class EventType : uint8_t
  {
    None,
    WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMove,
    AppTick, AppUpdate, AppRender,
    KeyPress, KeyRelease, KeyType,
    MouseButtonPress, MouseButtonRelease, MouseMove, MouseScroll
  };

  enum class EventCategory : uint8_t
  {
    None,
    Application  = BitUi8(0),
    Input        = BitUi8(1),
    Keyboard     = BitUi8(2),
    Mouse        = BitUi8(3),
    MouseButton  = BitUi8(4)
  };
  EN_ENABLE_BITMASK_OPERATORS(EventCategory);

  class Event
  {
  public:
    virtual ~Event() = default;

    bool handled = false;

    virtual EventType type() const = 0;
    virtual EventCategory categoryFlags() const = 0;
    virtual const char* name() const = 0;
    virtual std::string toString() const { return name(); }

    bool isInCategory(EventCategory category) { return static_cast<bool>(categoryFlags() & category); }
  };

  class EventDispatcher
  {
  public:
    EventDispatcher(Event& event)
      : m_Event(event) {}

    // T must be specified by caller.
    template<typename T, typename F, typename... Args>
      requires InvocableWithReturnType<F, bool, Args..., T&>
    bool dispatch(F&& eventFunction, Args&&... args)
    {
      if (m_Event.type() != T::Type())
        return false;

      if (!m_Event.handled)
        m_Event.handled = std::invoke(std::forward<F>(eventFunction), std::forward<Args>(args)..., static_cast<T&>(m_Event));
      return true;
    }

  private:
    Event& m_Event;
  };

  inline std::ostream& operator<<(std::ostream& os, const Event& e) { return os << e.toString(); }
}