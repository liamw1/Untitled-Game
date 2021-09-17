#pragma once
#include "KeyCodes.h"
#include "MouseButtonCodes.h"

/*
  Abstract representation of user input.
  Implementation is determined by derived class.
*/
namespace Engine
{
  class Input
  {
  public:
    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;

    inline static bool IsKeyPressed(Key keyCode) { return s_Instance->isKeyPressedImpl(keyCode); }
    inline static bool IsMouseButtonPressed(MouseButton button) { return s_Instance->isMouseButtonPressedImpl(button); }
    inline static std::array<float, 2> GetMousePosition() { return s_Instance->getMousePositionImpl(); }
    inline static float GetMouseX() { return s_Instance->getMouseXImpl(); }
    inline static float GetMouseY() { return s_Instance->getMouseYImpl(); }

  protected:
    Input() = default;

    virtual bool isKeyPressedImpl(Key keyCode) = 0;
    virtual bool isMouseButtonPressedImpl(MouseButton button) = 0;
    virtual std::array<float, 2> getMousePositionImpl() = 0;
    virtual float getMouseXImpl() = 0;
    virtual float getMouseYImpl() = 0;

  private:
    static Unique<Input> s_Instance;
  };
}