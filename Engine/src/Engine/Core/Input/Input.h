#pragma once
#include "KeyCodes.h"
#include "MouseButtonCodes.h"
#include "Engine/Math/Vec.h"

/*
  Abstract representation of player input.
  Platform-specific implementation is determined by derived class.
*/
namespace eng::input
{
  bool isKeyPressed(Key key);
  bool isMouseButtonPressed(Mouse button);
  
  math::Float2 getMousePosition();
  float getMouseX();
  float getMouseY();
}