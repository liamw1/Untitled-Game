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
    MouseMove(f32 x, f32 y);

    EventCategory categoryFlags() const;
    const char* name() const;
    std::string toString() const;

    f32 x() const;
    f32 y() const;

  private:
    f32 m_MouseX, m_MouseY;
  };

  class MouseScroll
  {
  public:
    MouseScroll(f32 xOffset, f32 yOffset);

    EventCategory categoryFlags() const;
    const char* name() const;
    std::string toString() const;

    f32 xOffset() const;
    f32 yOffset() const;

  private:
    f32 m_XOffset;
    f32 m_YOffset;
  };
}