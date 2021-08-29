#pragma once
#include "ENpch.h"

/*
  Abstract representation of user input.
  Implementation is determined by derived class.
*/
namespace Engine
{
  class ENGINE_API Input
  {
  public:
    inline static bool IsKeyPressed(int keyCode) { return Instance->isKeyPressedImpl(keyCode); }
    inline static bool IsMouseButtonPressed(int button) { return Instance->isMouseButtonPressedImpl(button); }
    inline static std::array<float, 2> GetMousePosition() { return Instance->getMousePositionImpl(); }
    inline static float GetMouseX() { return Instance->getMouseXImpl(); }
    inline static float GetMouseY() { return Instance->getMouseYImpl(); }

  protected:
    virtual bool isKeyPressedImpl(int keyCode) = 0;
    virtual bool isMouseButtonPressedImpl(int button) = 0;
    virtual std::array<float, 2> getMousePositionImpl() = 0;
    virtual float getMouseXImpl() = 0;
    virtual float getMouseYImpl() = 0;

  private:
    static Input* Instance;
  };
}