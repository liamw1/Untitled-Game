#pragma once
#include "KeyCodes.h"
#include "MouseButtonCodes.h"

/*
  Abstract representation of player input.
  Platform-specific implementation is determined by derived class.
*/
namespace Engine
{
  namespace Input
  {
    bool IsKeyPressed(Key keyCode);
    bool IsMouseButtonPressed(MouseButton button);

    Float2 GetMousePosition();
    float GetMouseX();
    float GetMouseY();
  };
}