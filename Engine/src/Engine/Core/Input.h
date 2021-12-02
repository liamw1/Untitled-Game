#pragma once
#include "KeyCodes.h"
#include "MouseButtonCodes.h"

/*
  Abstract representation of player input.
  Platform-specific implementation is determined by derived class.
*/
namespace Engine
{
  class Input
  {
  public:
    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;

    static bool IsKeyPressed(Key keyCode) { return s_Instance->isKeyPressedImpl(keyCode); }
    static bool IsMouseButtonPressed(MouseButton button) { return s_Instance->isMouseButtonPressedImpl(button); }
    static Float2 GetMousePosition() { return s_Instance->getMousePositionImpl(); }
    static float GetMouseX() { return s_Instance->getMouseXImpl(); }
    static float GetMouseY() { return s_Instance->getMouseYImpl(); }

  protected:
    Input() = default;

    virtual bool isKeyPressedImpl(Key keyCode) = 0;
    virtual bool isMouseButtonPressedImpl(MouseButton button) = 0;
    virtual Float2 getMousePositionImpl() = 0;
    virtual float getMouseXImpl() = 0;
    virtual float getMouseYImpl() = 0;

  private:
    static Unique<Input> s_Instance;
  };
}