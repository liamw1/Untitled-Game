#pragma once

/*
  A enum that denotes the category of an engine event.
*/
namespace eng::event
{
  enum class EventCategory
  {
    Application,
    Input,
    Keyboard,
    Mouse,
    MouseButton,

    First = 0, Last = MouseButton
  };
}