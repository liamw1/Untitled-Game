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
    int state = glfwGetKey(window, static_cast<int>(key));
    return state == GLFW_PRESS;
  }

  bool isMouseButtonPressed(Mouse button)
  {
    GLFWwindow* window = static_cast<GLFWwindow*>(Application::Get().getWindow().getNativeWindow());
    int state = glfwGetMouseButton(window, static_cast<int>(button));
    return state == GLFW_PRESS;
  }

  math::Float2 getMousePosition()
  {
    GLFWwindow* window = static_cast<GLFWwindow*>(Application::Get().getWindow().getNativeWindow());
    double xPos, yPos;
    glfwGetCursorPos(window, &xPos, &yPos);
    return { static_cast<float>(xPos), static_cast<float>(yPos) };
  }

  float getMouseX()
  {
    return getMousePosition().x;
  }

  float getMouseY()
  {
    return getMousePosition().y;
  }
}