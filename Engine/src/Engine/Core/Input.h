#pragma once
#include "VecTypes.h"
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
    bool IsKeyPressed(Key key);
    bool IsMouseButtonPressed(Mouse button);

    Float2 GetMousePosition();
    float GetMouseX();
    float GetMouseY();
  };
}