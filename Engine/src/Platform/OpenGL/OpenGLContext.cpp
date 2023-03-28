#include "ENpch.h"
#include "OpenGLContext.h"
#include "Engine/Renderer/GraphicsContext.h"
#include "Engine/Threading/Threads.h"
#include "Engine/Debug/Instrumentor.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

static constexpr bool c_VerboseOpenGLDebugLog = false;

namespace Engine
{
  static void openGLLogMessage(GLenum /*source*/, GLenum /*type*/, GLuint /*id*/, GLenum severity, GLsizei /*length*/, const GLchar* message, const void* /*userParam*/)
  {
    switch (severity)
    {
      case GL_DEBUG_SEVERITY_NOTIFICATION:  if (c_VerboseOpenGLDebugLog) EN_CORE_TRACE(message); return;
      case GL_DEBUG_SEVERITY_LOW:           EN_CORE_INFO(message);  return;
      case GL_DEBUG_SEVERITY_MEDIUM:        EN_CORE_WARN(message);  return;
      case GL_DEBUG_SEVERITY_HIGH:          EN_CORE_ERROR(message); return;
      default:                              EN_CORE_ASSERT(false, "OpenGL message \"{0}\" has unknown severity level", message); return;
    }
  }

  OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
    : m_WindowHandle(windowHandle)
  {
    EN_CORE_ASSERT(m_WindowHandle, "Window handle is null!");
  }

  void OpenGLContext::initialize()
  {
    EN_PROFILE_FUNCTION();
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::GetMainThreadID(), "OpenGL calls must be made in main thread!");

    glfwMakeContextCurrent(m_WindowHandle);
    int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    EN_CORE_ASSERT(status, "Failed to initialize GLad!");

    EN_CORE_INFO("OpenGL Info:");
    EN_CORE_INFO("  Vendor:    {0}", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    EN_CORE_INFO("  Renderer:  {0}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    EN_CORE_INFO("  Version:   {0}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    
    EN_CORE_ASSERT(GLVersion.major > 4 || (GLVersion.major == 4 && GLVersion.minor >= 5), "Hazel requires at least OpenGL version 4.5!");

    #ifdef EN_DEBUG
      // Initialize OpenGL debugging
      glEnable(GL_DEBUG_OUTPUT);
      glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
      glDebugMessageCallback(openGLLogMessage, nullptr);
    #endif
  }

  void OpenGLContext::swapBuffers()
  {
    EN_PROFILE_FUNCTION();
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::GetMainThreadID(), "OpenGL calls must be made in main thread!");

    glfwSwapBuffers(m_WindowHandle);
  }
}