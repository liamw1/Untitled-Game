#include "ENpch.h"
#include "Event.h"

namespace eng::event
{
  EnumBitMask<EventCategory> Event::categoryFlags() const
  {
    return std::visit([](auto&& rawEvent)
    {
      static_assert(requires(decltype(rawEvent) e) { { e.categoryFlags() } -> std::same_as<EnumBitMask<EventCategory>>; }, "Event type does not have a valid categoryFlags() function!");
      return rawEvent.categoryFlags();
    }, m_Event);
  }

  const char* Event::name() const
  {
    return std::visit([](auto&& rawEvent)
    {
      static_assert(requires(decltype(rawEvent) e) { { e.name() } -> std::same_as<const char*>; }, "Event type does not have a valid name() function!");
      return rawEvent.name();
    }, m_Event);
  }

  std::string Event::toString() const
  {
    return std::visit([](auto&& rawEvent) -> std::string
    {
      if constexpr (requires(decltype(rawEvent) e) { { e.toString() } -> std::same_as<std::string>; })
        return rawEvent.toString();
      else
        return rawEvent.name();
    }, m_Event);
  }

  bool Event::handled() const { return m_Handled; }
  void Event::flagAsHandled() { m_Handled = true; }
  bool Event::isInCategory(EventCategory category) { return categoryFlags()[category]; }
}
