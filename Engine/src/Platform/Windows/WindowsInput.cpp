#include "ENpch.h"
#include "WindowsInput.h"
#include "Engine/Core/Application.h"
#include <GLFW/glfw3.h>

namespace Engine
{
  Unique<Input> Input::s_Instance = CreateUnique<WindowsInput>();

  bool WindowsInput::isKeyPressedImpl(Key keyCode)
  {
    auto window = static_cast<GLFWwindow*>(Application::Get().getWindow().getNativeWindow());
    auto state = glfwGetKey(window, static_cast<int>(keyCode));
    return state == GLFW_PRESS || state == GLFW_REPEAT;
  }

  bool WindowsInput::isMouseButtonPressedImpl(MouseButton button)
  {
    auto window = static_cast<GLFWwindow*>(Application::Get().getWindow().getNativeWindow());
    auto state = glfwGetMouseButton(window, static_cast<int>(button));
    return state == GLFW_PRESS;
  }

  Float2 WindowsInput::getMousePositionImpl()
  {
    auto window = static_cast<GLFWwindow*>(Application::Get().getWindow().getNativeWindow());
    double xPos, yPos;
    glfwGetCursorPos(window, &xPos, &yPos);
    return { static_cast<float>(xPos), static_cast<float>(yPos) };
  }

  float WindowsInput::getMouseXImpl()
  {
    return getMousePositionImpl().x;
  }

  float WindowsInput::getMouseYImpl()
  {
    return getMousePositionImpl().y;
  }
}