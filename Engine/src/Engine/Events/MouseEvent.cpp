#include "ENpch.h"
#include "MouseEvent.h"

namespace eng::event
{
  MouseMove::MouseMove(float x, float y)
    : m_MouseX(x), m_MouseY(y) {}

  EventType MouseMove::type() const { return Type(); }
  EventType MouseMove::Type() { return EventType::MouseMove; }
  EventCategory MouseMove::categoryFlags() const { return EventCategory::Mouse | EventCategory::Input; }
  const char* MouseMove::name() const { return "MouseMove"; }
  std::string MouseMove::toString() const
  {
    std::stringstream ss;
    ss << "MouseMove: " << m_MouseX << ", " << m_MouseY;
    return ss.str();
  }

  float MouseMove::x() const { return m_MouseX; }
  float MouseMove::y() const { return m_MouseY; }



  MouseScroll::MouseScroll(float xOffset, float yOffset)
    : m_XOffset(xOffset), m_YOffset(yOffset) {}

  EventType MouseScroll::type() const { return Type(); }
  EventType MouseScroll::Type() { return EventType::MouseScroll; }
  EventCategory MouseScroll::categoryFlags() const { return EventCategory::Mouse | EventCategory::Input; }
  const char* MouseScroll::name() const { return "MouseScroll"; }
  std::string MouseScroll::toString() const
  {
    std::stringstream ss;
    ss << "MouseScroll: " << xOffset() << ", " << yOffset();
    return ss.str();
  }

  float MouseScroll::xOffset() const { return m_XOffset; }
  float MouseScroll::yOffset() const { return m_YOffset; }



  MouseButtonPress::MouseButtonPress(input::Mouse button)
    : m_Button(button) {}

  EventType MouseButtonPress::type() const { return Type(); }
  EventType MouseButtonPress::Type() { return EventType::MouseButtonPress; }
  EventCategory MouseButtonPress::categoryFlags() const { return EventCategory::Mouse | EventCategory::Input | EventCategory::MouseButton; }
  const char* MouseButtonPress::name() const { return "MouseButtonPress"; }
  std::string MouseButtonPress::toString() const
  {
    std::stringstream ss;
    ss << "MouseButtonPress: " << static_cast<input::mouseCode>(m_Button);
    return ss.str();
  }



  MouseButtonRelease::MouseButtonRelease(input::Mouse button)
    : m_Button(button) {}

  EventType MouseButtonRelease::type() const { return Type(); }
  EventType MouseButtonRelease::Type() { return EventType::MouseButtonRelease; }
  EventCategory MouseButtonRelease::categoryFlags() const { return EventCategory::Mouse | EventCategory::Input | EventCategory::MouseButton; }
  const char* MouseButtonRelease::name() const { return "MouseButtonRelease"; }
  std::string MouseButtonRelease::toString() const
  {
    std::stringstream ss;
    ss << "MouseButtonRelease: " << static_cast<input::mouseCode>(m_Button);
    return ss.str();
  }
}