#include "ENpch.h"
#include "WindowsInput.h"
#include "Engine/Core/Application.h"
#include <GLFW/glfw3.h>

namespace Engine
{
  Input* Input::Instance = new WindowsInput();

  bool WindowsInput::isKeyPressedImpl(int keyCode)
  {
    auto window = static_cast<GLFWwindow*>(Application::Get().getWindow().getNativeWindow());
    auto state = glfwGetKey(window, keyCode);
    return state == GLFW_PRESS || state == GLFW_REPEAT;
  }
  bool WindowsInput::isMouseButtonPressedImpl(int button)
  {
    auto window = static_cast<GLFWwindow*>(Application::Get().getWindow().getNativeWindow());
    auto state = glfwGetMouseButton(window, button);
    return state == GLFW_PRESS;
  }
  std::array<float, 2> WindowsInput::getMousePositionImpl()
  {
    auto window = static_cast<GLFWwindow*>(Application::Get().getWindow().getNativeWindow());
    double xPos, yPos;
    glfwGetCursorPos(window, &xPos, &yPos);
    return { (float)xPos, (float)yPos };
  }
  float WindowsInput::getMouseXImpl()
  {
    auto [x, y] = getMousePositionImpl();
    return x;
  }
  float WindowsInput::getMouseYImpl()
  {
    auto [x, y] = getMousePositionImpl();
    return y;
  }
}