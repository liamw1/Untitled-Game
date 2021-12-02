#pragma once
#include "Engine/Core/Input.h"

namespace Engine
{
  class WindowsInput : public Input
  {
  protected:
    bool isKeyPressedImpl(Key keyCode) override;
    bool isMouseButtonPressedImpl(MouseButton button) override;
    Float2 getMousePositionImpl() override;
    float getMouseXImpl() override;
    float getMouseYImpl() override;
  };
}