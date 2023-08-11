#pragma once
#include "KeyCodes.h"
#include "MouseButtonCodes.h"
#include "Engine/Math/Vec.h"

/*
  Abstract representation of player input.
  Platform-specific implementation is determined by derived class.
*/
namespace Engine::Input
{
  bool IsKeyPressed(Key key);
  bool IsMouseButtonPressed(Mouse button);
  
  Float2 GetMousePosition();
  float GetMouseX();
  float GetMouseY();
}