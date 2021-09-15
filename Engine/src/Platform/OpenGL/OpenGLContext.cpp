#include "ENpch.h"
#include "OpenGLContext.h"
#include "Engine/Renderer/GraphicsContext.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace Engine
{
  static void openGLLogMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
  {
    switch (severity)
    {
      case GL_DEBUG_SEVERITY_HIGH:          EN_CORE_ERROR(message); break;
      case GL_DEBUG_SEVERITY_MEDIUM:        EN_CORE_WARN(message); break;
      case GL_DEBUG_SEVERITY_LOW:           EN_CORE_INFO(message); break;
      case GL_DEBUG_SEVERITY_NOTIFICATION:  EN_CORE_TRACE(message); break;
    }
  }

  OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
    : m_WindowHandle(windowHandle)
  {
    EN_CORE_ASSERT(m_WindowHandle, "Window handle is null!");
  }

  void OpenGLContext::initialize()
  {
    glfwMakeContextCurrent(m_WindowHandle);
    int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    EN_CORE_ASSERT(status, "Failed to initialize GLad!");

    EN_CORE_INFO("OpenGL Info:");
    EN_CORE_INFO("  Vendor:    {0}", glGetString(GL_VENDOR));
    EN_CORE_INFO("  Renderer:  {0}", glGetString(GL_RENDERER));
    EN_CORE_INFO("  Version:   {0}", glGetString(GL_VERSION));

    // Initialize OpenGL debugging
    glDebugMessageCallback(openGLLogMessage, nullptr);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  }

  void OpenGLContext::swapBuffers()
  {
    glfwSwapBuffers(m_WindowHandle);
  }
}