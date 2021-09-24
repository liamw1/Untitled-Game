#include "ENpch.h"
#include "WindowsWindow.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Events/KeyEvent.h"
#include "Engine/Events/MouseEvent.h"
#include "Engine/Events/ApplicationEvent.h"
#include "Platform/OpenGL/OpenGLContext.h"

namespace Engine
{
  static uint8_t GLFWWindowCount = 0;

  static void GLFWErrorCallback(int errorCode, const char* description)
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
    EN_PROFILE_FUNCTION();

    shutdown();
  }

  void WindowsWindow::onUpdate()
  {
    EN_PROFILE_FUNCTION();

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

  void WindowsWindow::enableCursor()
  {
    glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  }

  void WindowsWindow::disableCursor()
  {
    glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported())
      glfwSetInputMode(m_Window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
  }

  void WindowsWindow::initialize(const WindowProps& properties)
  {
    EN_PROFILE_FUNCTION();

    m_Data.title = properties.title;
    m_Data.width = properties.width;
    m_Data.height = properties.height;

    EN_CORE_INFO("Creating window {0} ({1}, {2})", properties.title, properties.width, properties.height);
    
    if (GLFWWindowCount == 0)
    {
      EN_PROFILE_SCOPE("glfwInit");
      int success = glfwInit();
      EN_CORE_ASSERT(success, "Could not initialize GLFW!");
      glfwSetErrorCallback(GLFWErrorCallback);
    }

    {
      EN_PROFILE_SCOPE("glfwCreateWindow");

      #if defined(EN_DEBUG)
        if (Renderer::GetAPI() == RendererAPI::API::OpenGL)
          glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
      #endif
      m_Window = glfwCreateWindow((int)properties.width, (int)properties.height, m_Data.title.c_str(), nullptr, nullptr);
      ++GLFWWindowCount;
    }

    m_Context = createUnique<OpenGLContext>(m_Window);
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

    auto keyCallback = [](GLFWwindow* window, int keyCode, int /*scanCode*/, int action, int /*mods*/)
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

    auto charCallback = [](GLFWwindow* window, uint32_t keycode)
    {
      WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
      KeyTypeEvent event((Key)keycode);
      data.eventCallback(event);
    };
    glfwSetCharCallback(m_Window, charCallback);

    auto mouseButtonCallback = [](GLFWwindow* window, int button, int action, int /*mods*/)
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
    EN_PROFILE_FUNCTION();

    glfwDestroyWindow(m_Window);
    --GLFWWindowCount;

    if (GLFWWindowCount == 0)
      glfwTerminate();
  }
}
