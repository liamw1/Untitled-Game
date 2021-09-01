#include "ENpch.h"
#include "OpenGLContext.h"
#include "Engine/Renderer/GraphicsContext.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace Engine
{
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
  }

  void OpenGLContext::swapBuffers()
  {
    glfwSwapBuffers(m_WindowHandle);
  }
}