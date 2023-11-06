#pragma once
#include "Engine/Utilities/BitUtilities.h"
#include "Engine/Utilities/EnumUtilities.h"

/*
  A enum that denotes the category of an engine event.
  Multiple categories can be specified by bit-masking.

  Example:
    EventCategory keyboardInput = EventCategory::Input | EventCategory::Keyboard;
    bool isKeyboardEvent = keyboardInput & EventCategory::Keyboard;
*/
namespace eng::event
{
  enum class EventCategory
  {
    None,
    Application = u8Bit(0),
    Input = u8Bit(1),
    Keyboard = u8Bit(2),
    Mouse = u8Bit(3),
    MouseButton = u8Bit(4)
  };
  ENG_ENABLE_BITMASK_OPERATORS(EventCategory);
}