#include "ENpch.h"
#include "WindowsWindow.h"
#include "Engine/Events/KeyEvent.h"
#include "Engine/Events/MouseEvent.h"
#include "Engine/Events/ApplicationEvent.h"

namespace Engine
{
  static bool GLFWInitialized = false;

  static inline void GLFWErrorCallback(int errorCode, const char* description)
  {
    EN_CORE_ERROR("GLFW Error ({0}): {1}", errorCode, description);
  }

  Window* Window::Create(const WindowProps& props)
  {
    return new WindowsWindow(props);
  }

  WindowsWindow::WindowsWindow(const WindowProps& properties)
  {
    initialize(properties);
  }

  WindowsWindow::~WindowsWindow()
  {
  }

  void WindowsWindow::onUpdate()
  {
    glfwPollEvents();
    glfwSwapBuffers(m_Window);
  }

  void WindowsWindow::setVSync(bool enabled)
  {
    if (enabled)
      glfwSwapInterval(1);
    else
      glfwSwapInterval(0);

    m_Data.VSync = enabled;
  }

  bool WindowsWindow::isVSync() const
  {
    return m_Data.VSync;
  }

  void WindowsWindow::initialize(const WindowProps& properties)
  {
    m_Data.title = properties.title;
    m_Data.width = properties.width;
    m_Data.height = properties.height;

    EN_CORE_INFO("Creating window {0} ({1}, {2})", properties.title, properties.width, properties.height);

    if (!GLFWInitialized)
    {
      // TODO: glfwTerminate on system shutdown
      int success = glfwInit();
      EN_CORE_ASSERT(success, "Could not initialize GLFW!");

      glfwSetErrorCallback(GLFWErrorCallback);

      GLFWInitialized = true;
    }

    m_Window = glfwCreateWindow((int)properties.width, (int)properties.height, m_Data.title.c_str(), nullptr, nullptr);
    glfwMakeContextCurrent(m_Window);
    glfwSetWindowUserPointer(m_Window, &m_Data);
    setVSync(true);

    // Set GLFW callbacks
    auto windowSizeCallback = [](GLFWwindow* window, int width, int height)
    {
      WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
      data.width = width;
      data.height = height;

      WindowResizeEvent event(width, height);
      data.eventCallback(event);
    };
    glfwSetWindowSizeCallback(m_Window, windowSizeCallback);

    auto windowCloseCallback = [](GLFWwindow* window)
    {
      WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
      WindowCloseEvent event;
      data.eventCallback(event);
    };
    glfwSetWindowCloseCallback(m_Window, windowCloseCallback);

    auto keyCallback = [](GLFWwindow* window, int key, int scancode, int action, int mods)
    {
      WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

      switch (action)
      {
        case GLFW_PRESS:
        {
          KeyPressedEvent event(key, 0);
          data.eventCallback(event);
          break;
        }
        case GLFW_RELEASE:
        {
          KeyReleasedEvent event(key);
          data.eventCallback(event);
          break;
        }
        case GLFW_REPEAT:
        {
          KeyPressedEvent event(key, 1);
          data.eventCallback(event);
          break;
        }
      }
    };
    glfwSetKeyCallback(m_Window, keyCallback);

    auto mouseButtonCallback = [](GLFWwindow* window, int button, int action, int mods)
    {
      WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

      switch (action)
      {
        case GLFW_PRESS:
        {
          MouseButtonPressedEvent event(button);
          data.eventCallback(event);
          break;
        }
        case GLFW_RELEASE:
        {
          MouseButtonReleasedEvent event(button);
          data.eventCallback(event);
          break;
        }
      }
    };
    glfwSetMouseButtonCallback(m_Window, mouseButtonCallback);

    auto mouseScrollCallback = [](GLFWwindow* window, double xOffset, double yOffset)
    {
      WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
      MouseScrolledEvent event((float)xOffset, (float)yOffset);
      data.eventCallback(event);
    };
    glfwSetScrollCallback(m_Window, mouseScrollCallback);

    auto cursorPosCallback = [](GLFWwindow* window, double xPos, double yPos)
    {
      WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
      MouseMovedEvent event((float)xPos, (float)yPos);
      data.eventCallback(event);
    };
    glfwSetCursorPosCallback(m_Window, cursorPosCallback);
  }

  void WindowsWindow::shutdown()
  {
    glfwDestroyWindow(m_Window);
  }
}
