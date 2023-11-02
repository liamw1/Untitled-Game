#include "ENpch.h"
#include "WindowsWindow.h"
#include "Engine/Renderer/RendererAPI.h"
#include "Engine/Events/KeyEvent.h"
#include "Engine/Events/MouseEvent.h"
#include "Engine/Events/ApplicationEvent.h"
#include "Platform/OpenGL/OpenGLContext.h"
#include "Engine/Debug/Instrumentor.h"

namespace eng
{
  static i32 GLFWWindowCount = 0;

  static void GLFWErrorCallback(i32 errorCode, const char* description)
  {
    ENG_CORE_ERROR("GLFW Error ({0}): {1}", errorCode, description);
  }

  WindowsWindow::WindowsWindow(const WindowProps& properties)
  {
    initialize(properties);
  }

  WindowsWindow::~WindowsWindow()
  {
    ENG_PROFILE_FUNCTION();

    shutdown();
  }

  void WindowsWindow::onUpdate()
  {
    ENG_PROFILE_FUNCTION();

    glfwPollEvents();
    m_Context->swapBuffers();
  }

  u32 WindowsWindow::getWidth() const { return m_Data.width; }
  u32 WindowsWindow::getHeight() const { return m_Data.height; }

  void WindowsWindow::setEventCallback(const EventCallbackFn& callback)
  {
    m_Data.eventCallback = callback;
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

  void* WindowsWindow::getNativeWindow() const
  {
    return m_Window;
  }

  void WindowsWindow::initialize(const WindowProps& properties)
  {
    ENG_PROFILE_FUNCTION();

    m_Data.title = properties.title;
    m_Data.width = properties.width;
    m_Data.height = properties.height;

    ENG_CORE_INFO("Creating window {0} ({1}, {2})", properties.title, properties.width, properties.height);
    
    if (GLFWWindowCount == 0)
    {
      ENG_PROFILE_SCOPE("glfwInit");
      i32 success = glfwInit();
      ENG_CORE_ASSERT(success, "Could not initialize GLFW!");
      glfwSetErrorCallback(GLFWErrorCallback);
    }

    {
      ENG_PROFILE_SCOPE("glfwCreateWindow");

      #if defined(ENG_DEBUG)
        if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL || RendererAPI::GetAPI() == RendererAPI::API::OpenGL_Legacy)
          glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
      #endif
      m_Window = glfwCreateWindow(arithmeticCast<i32>(properties.width), arithmeticCast<i32>(properties.height), m_Data.title.c_str(), nullptr, nullptr);
      ++GLFWWindowCount;
    }

    m_Context = std::make_unique<OpenGLContext>(m_Window);
    m_Context->initialize();

    glfwSetWindowUserPointer(m_Window, &m_Data);
    setVSync(true);

    // Set GLFW callbacks
    auto windowSizeCallback = [](GLFWwindow* window, i32 width, i32 height)
    {
      WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
      data.width = width;
      data.height = height;

      event::Event event = event::WindowResize(width, height);
      data.eventCallback(event);
    };
    glfwSetWindowSizeCallback(m_Window, windowSizeCallback);

    auto windowCloseCallback = [](GLFWwindow* window)
    {
      WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
      event::Event event = event::WindowClose();
      data.eventCallback(event);
    };
    glfwSetWindowCloseCallback(m_Window, windowCloseCallback);

    auto keyCallback = [](GLFWwindow* window, i32 keyCode, i32 /*scanCode*/, i32 action, i32 /*mods*/)
    {
      WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

      switch (action)
      {
        case GLFW_PRESS:
        {
          event::Event event = event::KeyPress(enumCast<input::Key>(keyCode), false);
          data.eventCallback(event);
          break;
        }
        case GLFW_RELEASE:
        {
          event::Event event = event::KeyRelease(enumCast<input::Key>(keyCode));
          data.eventCallback(event);
          break;
        }
        case GLFW_REPEAT:
        {
          event::Event event = event::KeyPress(enumCast<input::Key>(keyCode));
          data.eventCallback(event);
          break;
        }
      }
    };
    glfwSetKeyCallback(m_Window, keyCallback);

    auto charCallback = [](GLFWwindow* window, u32 keyCode)
    {
      WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
      event::Event event = event::KeyType(enumCast<input::Key>(keyCode));
      data.eventCallback(event);
    };
    glfwSetCharCallback(m_Window, charCallback);

    auto mouseButtonCallback = [](GLFWwindow* window, i32 button, i32 action, i32 /*mods*/)
    {
      WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

      switch (action)
      {
        case GLFW_PRESS:
        {
          event::Event event = event::MouseButtonPress(enumCast<input::Mouse>(button));
          data.eventCallback(event);
          break;
        }
        case GLFW_RELEASE:
        {
          event::Event event = event::MouseButtonRelease(enumCast<input::Mouse>(button));
          data.eventCallback(event);
          break;
        }
      }
    };
    glfwSetMouseButtonCallback(m_Window, mouseButtonCallback);

    auto mouseScrollCallback = [](GLFWwindow* window, f64 xOffset, f64 yOffset)
    {
      WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
      event::Event event = event::MouseScroll(arithmeticCastUnchecked<f32>(xOffset), arithmeticCastUnchecked<f32>(yOffset));
      data.eventCallback(event);
    };
    glfwSetScrollCallback(m_Window, mouseScrollCallback);

    auto cursorPosCallback = [](GLFWwindow* window, f64 xPos, f64 yPos)
    {
      WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
      event::Event event = event::MouseMove(arithmeticCastUnchecked<f32>(xPos), arithmeticCastUnchecked<f32>(yPos));
      data.eventCallback(event);
    };
    glfwSetCursorPosCallback(m_Window, cursorPosCallback);
  }

  void WindowsWindow::shutdown()
  {
    ENG_PROFILE_FUNCTION();

    glfwDestroyWindow(m_Window);
    --GLFWWindowCount;

    if (GLFWWindowCount == 0)
      glfwTerminate();
  }
}
