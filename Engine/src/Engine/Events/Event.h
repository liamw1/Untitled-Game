#pragma once
#include "ApplicationEvent.h"
#include "KeyEvent.h"
#include "MouseEvent.h"
#include "Engine/Utilities/Constraints.h"

/*
  NOTE:
  Events in Engine are currently blocking, meaning when an event occurs it
  immediately gets dispatched and must be dealt with right then and there.
  For the future, a better strategy might be to buffer events in an event
  bus and process them during the "event" part of the update stage.
*/
namespace eng::event
{
  class Event : private NonCopyable, NonMovable
  {
    using EventVariant = std::variant<AppRender, AppTick, AppUpdate, WindowClose, WindowResize,
                                      MouseButtonPress, MouseButtonRelease, MouseMove, MouseScroll,
                                      KeyPress, KeyRelease, KeyType>;

    EventVariant m_Event;
    bool m_Handled;

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
      std::visit([this, &eventFunction, &args...](auto&& rawEvent)
      {
        static_assert(ValidEventFunction<F, Args...>(), "Given function is not a valid event function!");

        using EventType = decltype(rawEvent);
        if constexpr (EventFunctionFor<EventType, F, Args...>())
          if (!m_Handled)
            m_Handled = std::invoke(std::forward<F>(eventFunction), std::forward<Args>(args)..., std::forward<EventType>(rawEvent));
      }, m_Event);
    }

  private:
    template<typename EventType, typename F, typename... Args>
    static constexpr bool EventFunctionFor() { return std::is_invocable_r_v<bool, F, Args..., EventType&>; }

    template<typename F, typename... Args>
    static constexpr bool ValidEventFunction() { return CheckEventTypeAt<0, F, Args...>(); }

    template<i32 N, typename F, typename... Args>
    static constexpr bool CheckEventTypeAt()
    {
      if constexpr (N < std::variant_size_v<EventVariant>)
      {
        if constexpr (EventFunctionFor<std::variant_alternative_t<N, EventVariant>, F, Args...>())
          return true;
        else
          return CheckEventTypeAt<N + 1, F, Args...>();
      }
      return false;
    }
  };

  inline std::ostream& operator<<(std::ostream& os, const Event& e) { return os << e.toString(); }
}