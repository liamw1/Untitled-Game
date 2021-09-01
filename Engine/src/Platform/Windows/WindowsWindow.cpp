#include "ENpch.h"
#include "WindowsWindow.h"
#include "Engine/Events/KeyEvent.h"
#include "Engine/Events/MouseEvent.h"
#include "Engine/Events/ApplicationEvent.h"
#include "Platform/OpenGL/OpenGLContext.h"

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
    m_Context->swapBuffers();
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
    m_Context = new OpenGLContext(m_Window);
    m_Context->initialize();

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

    auto keyCallback = [](GLFWwindow* window, int keyCode, int scancode, int action, int mods)
    {
      WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

      switch (action)
      {
        case GLFW_PRESS:
        {
          KeyPressEvent event((Key)keyCode, 0);
          data.eventCallback(event);
          break;
        }
        case GLFW_RELEASE:
        {
          KeyReleaseEvent event((Key)keyCode);
          data.eventCallback(event);
          break;
        }
        case GLFW_REPEAT:
        {
          KeyPressEvent event((Key)keyCode, 1);
          data.eventCallback(event);
          break;
        }
      }
    };
    glfwSetKeyCallback(m_Window, keyCallback);

    auto charCallback = [](GLFWwindow* window, unsigned int keycode)
    {
      WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
      KeyTypeEvent event((Key)keycode);
      data.eventCallback(event);
    };
    glfwSetCharCallback(m_Window, charCallback);

    auto mouseButtonCallback = [](GLFWwindow* window, int button, int action, int mods)
    {
      WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

      switch (action)
      {
        case GLFW_PRESS:
        {
          MouseButtonPressEvent event((MouseButton)button);
          data.eventCallback(event);
          break;
        }
        case GLFW_RELEASE:
        {
          MouseButtonReleaseEvent event((MouseButton)button);
          data.eventCallback(event);
          break;
        }
      }
    };
    glfwSetMouseButtonCallback(m_Window, mouseButtonCallback);

    auto mouseScrollCallback = [](GLFWwindow* window, double xOffset, double yOffset)
    {
      WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
      MouseScrollEvent event((float)xOffset, (float)yOffset);
      data.eventCallback(event);
    };
    glfwSetScrollCallback(m_Window, mouseScrollCallback);

    auto cursorPosCallback = [](GLFWwindow* window, double xPos, double yPos)
    {
      WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
      MouseMoveEvent event((float)xPos, (float)yPos);
      data.eventCallback(event);
    };
    glfwSetCursorPosCallback(m_Window, cursorPosCallback);
  }

  void WindowsWindow::shutdown()
  {
    glfwDestroyWindow(m_Window);
  }
}
