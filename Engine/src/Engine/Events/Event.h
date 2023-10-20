#pragma once
#include "ApplicationEvent.h"
#include "KeyEvent.h"
#include "MouseEvent.h"
#include "Engine/Utilities/Constraints.h"

namespace eng::event
{
  /*
    NOTE:
    Events in Engine are currently blocking, meaning when an event occurs it
    immediately gets dispatched and must be dealt with right then and there.
    For the future, a better strategy might be to buffer events in an event
    bus and process them during the "event" part of the update stage.
  */

  class Event : private NonCopyable, NonMovable
  {
  public:
    template<typename T>
    Event(const T& e)
      : m_Event(e), m_Handled(false) {}

    EventCategory categoryFlags() const;
    const char* name() const;
    std::string toString() const;

    bool handled() const;
    void flagAsHandled();
    bool isInCategory(EventCategory category);

    template<typename F, typename... Args>
    void dispatch(F&& eventFunction, Args&&... args)
    {
      std::visit([this, eventFunction = std::forward<F>(eventFunction), ...args = std::forward<Args>(args)](auto&& rawEvent)
      {
        // TODO: Static assert here if function is not invocable with any event type

        using EventType = decltype(rawEvent);
        if constexpr (std::is_invocable_v<F, Args..., EventType>)
          if (!m_Handled)
            m_Handled = std::invoke(eventFunction, args..., std::forward<EventType>(rawEvent));
      }, m_Event);
    }

  private:
    using EventVariant = std::variant<AppRender, AppTick, AppUpdate, WindowClose, WindowResize,
                                      KeyPress, KeyRelease, KeyType,
                                      MouseButtonPress, MouseButtonRelease, MouseMove, MouseScroll>;
    EventVariant m_Event;
    bool m_Handled;
  };

  inline std::ostream& operator<<(std::ostream& os, const Event& e) { return os << e.toString(); }
}