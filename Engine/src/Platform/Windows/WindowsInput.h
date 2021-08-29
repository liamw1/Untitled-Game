#pragma once
#include "ENpch.h"
#include "Engine/Core/Input.h"

namespace Engine
{
  class WindowsInput : public Input
  {
  protected:
    bool isKeyPressedImpl(int keyCode) override;
    bool isMouseButtonPressedImpl(int button) override;
    std::array<float, 2> getMousePositionImpl() override;
    float getMouseXImpl() override;
    float getMouseYImpl() override;
  };
}