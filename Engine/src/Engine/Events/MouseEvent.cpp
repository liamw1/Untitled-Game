#include "ENpch.h"
#include "MouseEvent.h"

namespace Engine
{
  MouseMoveEvent::MouseMoveEvent(float x, float y)
    : m_MouseX(x), m_MouseY(y) {}

  EventType MouseMoveEvent::type() const { return Type(); }
  EventType MouseMoveEvent::Type() { return EventType::MouseMove; }
  EventCategory MouseMoveEvent::categoryFlags() const { return EventCategory::Mouse | EventCategory::Input; }
  const char* MouseMoveEvent::name() const { return "MouseMove"; }
  std::string MouseMoveEvent::toString() const
  {
    std::stringstream ss;
    ss << "MouseMoveEvent: " << m_MouseX << ", " << m_MouseY;
    return ss.str();
  }

  float MouseMoveEvent::x() const { return m_MouseX; }
  float MouseMoveEvent::y() const { return m_MouseY; }



  MouseScrollEvent::MouseScrollEvent(float xOffset, float yOffset)
    : m_XOffset(xOffset), m_YOffset(yOffset) {}

  EventType MouseScrollEvent::type() const { return Type(); }
  EventType MouseScrollEvent::Type() { return EventType::MouseScroll; }
  EventCategory MouseScrollEvent::categoryFlags() const { return EventCategory::Mouse | EventCategory::Input; }
  const char* MouseScrollEvent::name() const { return "MouseScroll"; }
  std::string MouseScrollEvent::toString() const
  {
    std::stringstream ss;
    ss << "MouseScrollEvent: " << xOffset() << ", " << yOffset();
    return ss.str();
  }

  float MouseScrollEvent::xOffset() const { return m_XOffset; }
  float MouseScrollEvent::yOffset() const { return m_YOffset; }



  MouseButtonPressEvent::MouseButtonPressEvent(Mouse button)
    : m_Button(button) {}

  EventType MouseButtonPressEvent::type() const { return Type(); }
  EventType MouseButtonPressEvent::Type() { return EventType::MouseButtonPress; }
  EventCategory MouseButtonPressEvent::categoryFlags() const { return EventCategory::Mouse | EventCategory::Input | EventCategory::MouseButton; }
  const char* MouseButtonPressEvent::name() const { return "MouseButtonPress"; }
  std::string MouseButtonPressEvent::toString() const
  {
    std::stringstream ss;
    ss << "MouseButtonPressEvent: " << static_cast<mouseCode>(m_Button);
    return ss.str();
  }



  MouseButtonReleaseEvent::MouseButtonReleaseEvent(Mouse button)
    : m_Button(button) {}

  EventType MouseButtonReleaseEvent::type() const { return Type(); }
  EventType MouseButtonReleaseEvent::Type() { return EventType::MouseButtonRelease; }
  EventCategory MouseButtonReleaseEvent::categoryFlags() const { return EventCategory::Mouse | EventCategory::Input | EventCategory::MouseButton; }
  const char* MouseButtonReleaseEvent::name() const { return "MouseButtonRelease"; }
  std::string MouseButtonReleaseEvent::toString() const
  {
    std::stringstream ss;
    ss << "MouseButtonReleaseEvent: " << static_cast<mouseCode>(m_Button);
    return ss.str();
  }
}