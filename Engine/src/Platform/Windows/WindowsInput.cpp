#include "ENpch.h"
#include "Engine/Core/Input/Input.h"
#include "Engine/Core/Application.h"
#include "Engine/Core/Window.h"
#include <GLFW/glfw3.h>

namespace eng::input
{
  bool isKeyPressed(Key key)
  {
    GLFWwindow* window = static_cast<GLFWwindow*>(Application::Get().getWindow().getNativeWindow());
    i32 state = glfwGetKey(window, static_cast<i32>(key));
    return state == GLFW_PRESS;
  }

  bool isMouseButtonPressed(Mouse button)
  {
    GLFWwindow* window = static_cast<GLFWwindow*>(Application::Get().getWindow().getNativeWindow());
    i32 state = glfwGetMouseButton(window, static_cast<i32>(button));
    return state == GLFW_PRESS;
  }

  math::Float2 getMousePosition()
  {
    GLFWwindow* window = static_cast<GLFWwindow*>(Application::Get().getWindow().getNativeWindow());
    f64 xPos, yPos;
    glfwGetCursorPos(window, &xPos, &yPos);
    return { static_cast<f32>(xPos), static_cast<f32>(yPos) };
  }

  f32 getMouseX()
  {
    return getMousePosition().x;
  }

  f32 getMouseY()
  {
    return getMousePosition().y;
  }
}