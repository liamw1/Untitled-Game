#pragma once
#include "EventCategory.h"
#include "Engine/Core/Input/MouseButtonCodes.h"

namespace eng::event
{
  class MouseButtonPress
  {
  public:
    MouseButtonPress(input::Mouse button);

    EventCategory categoryFlags() const;
    const char* name() const;
    std::string toString() const;

  private:
    input::Mouse m_Button;
  };

  class MouseButtonRelease
  {
  public:
    MouseButtonRelease(input::Mouse button);

    EventCategory categoryFlags() const;
    const char* name() const;
    std::string toString() const;

  private:
    input::Mouse m_Button;
  };

  class MouseMove
  {
  public:
    MouseMove(float x, float y);

    EventCategory categoryFlags() const;
    const char* name() const;
    std::string toString() const;

    float x() const;
    float y() const;

  private:
    float m_MouseX, m_MouseY;
  };

  class MouseScroll
  {
  public:
    MouseScroll(float xOffset, float yOffset);

    EventCategory categoryFlags() const;
    const char* name() const;
    std::string toString() const;

    float xOffset() const;
    float yOffset() const;

  private:
    float m_XOffset, m_YOffset;
  };
}