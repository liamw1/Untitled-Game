#include "ENpch.h"
#include "Engine/Core/Input.h"
#include "Engine/Core/Application.h"
#include <GLFW/glfw3.h>

namespace Engine
{
  bool Input::IsKeyPressed(Key key)
  {
    GLFWwindow* window = static_cast<GLFWwindow*>(Application::Get().getWindow().getNativeWindow());
    int state = glfwGetKey(window, static_cast<int>(key));
    return state == GLFW_PRESS;
  }

  bool Input::IsMouseButtonPressed(Mouse button)
  {
    GLFWwindow* window = static_cast<GLFWwindow*>(Application::Get().getWindow().getNativeWindow());
    int state = glfwGetMouseButton(window, static_cast<int>(button));
    return state == GLFW_PRESS;
  }

  Float2 Input::GetMousePosition()
  {
    GLFWwindow* window = static_cast<GLFWwindow*>(Application::Get().getWindow().getNativeWindow());
    double xPos, yPos;
    glfwGetCursorPos(window, &xPos, &yPos);
    return { static_cast<float>(xPos), static_cast<float>(yPos) };
  }

  float Input::GetMouseX()
  {
    return GetMousePosition().x;
  }

  float Input::GetMouseY()
  {
    return GetMousePosition().y;
  }
}