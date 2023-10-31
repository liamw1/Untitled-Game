#include "ENpch.h"
#include "MouseEvent.h"

namespace eng::event
{
  MouseButtonPress::MouseButtonPress(input::Mouse button)
    : m_Button(button) {}

  EventCategory MouseButtonPress::categoryFlags() const { return EventCategory::Mouse | EventCategory::Input | EventCategory::MouseButton; }
  const char* MouseButtonPress::name() const { return "MouseButtonPress"; }
  std::string MouseButtonPress::toString() const
  {
    std::stringstream ss;
    ss << "MouseButtonPress: " << toUnderlying(m_Button);
    return ss.str();
  }



  MouseButtonRelease::MouseButtonRelease(input::Mouse button)
    : m_Button(button) {}

  EventCategory MouseButtonRelease::categoryFlags() const { return EventCategory::Mouse | EventCategory::Input | EventCategory::MouseButton; }
  const char* MouseButtonRelease::name() const { return "MouseButtonRelease"; }
  std::string MouseButtonRelease::toString() const
  {
    std::stringstream ss;
    ss << "MouseButtonRelease: " << toUnderlying(m_Button);
    return ss.str();
  }



  MouseMove::MouseMove(f32 x, f32 y)
    : m_MouseX(x), m_MouseY(y) {}

  EventCategory MouseMove::categoryFlags() const { return EventCategory::Mouse | EventCategory::Input; }
  const char* MouseMove::name() const { return "MouseMove"; }
  std::string MouseMove::toString() const
  {
    std::stringstream ss;
    ss << "MouseMove: " << m_MouseX << ", " << m_MouseY;
    return ss.str();
  }

  f32 MouseMove::x() const { return m_MouseX; }
  f32 MouseMove::y() const { return m_MouseY; }



  MouseScroll::MouseScroll(f32 xOffset, f32 yOffset)
    : m_XOffset(xOffset), m_YOffset(yOffset) {}

  EventCategory MouseScroll::categoryFlags() const { return EventCategory::Mouse | EventCategory::Input; }
  const char* MouseScroll::name() const { return "MouseScroll"; }
  std::string MouseScroll::toString() const
  {
    std::stringstream ss;
    ss << "MouseScroll: " << xOffset() << ", " << yOffset();
    return ss.str();
  }

  f32 MouseScroll::xOffset() const { return m_XOffset; }
  f32 MouseScroll::yOffset() const { return m_YOffset; }
}