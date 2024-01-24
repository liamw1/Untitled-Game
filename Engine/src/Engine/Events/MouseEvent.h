#pragma once
#include "EventCategory.h"
#include "Engine/Core/Input/MouseButtonCodes.h"
#include "Engine/Utilities/EnumUtilities.h"

namespace eng::event
{
  class MouseButtonPress
  {
    input::Mouse m_Button;

  public:
    MouseButtonPress(input::Mouse button);

    EnumBitMask<EventCategory> categoryFlags() const;
    std::string_view name() const;
    std::string toString() const;
  };

  class MouseButtonRelease
  {
    input::Mouse m_Button;

  public:
    MouseButtonRelease(input::Mouse button);

    EnumBitMask<EventCategory> categoryFlags() const;
    std::string_view name() const;
    std::string toString() const;
  };

  class MouseMove
  {
    f32 m_MouseX;
    f32 m_MouseY;

  public:
    MouseMove(f32 x, f32 y);

    EnumBitMask<EventCategory> categoryFlags() const;
    std::string_view name() const;
    std::string toString() const;

    f32 x() const;
    f32 y() const;
  };

  class MouseScroll
  {
    f32 m_XOffset;
    f32 m_YOffset;

  public:
    MouseScroll(f32 xOffset, f32 yOffset);

    EnumBitMask<EventCategory> categoryFlags() const;
    std::string_view name() const;
    std::string toString() const;

    f32 xOffset() const;
    f32 yOffset() const;
  };
}