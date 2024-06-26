#include "ENpch.h"
#include "OpenGLContext.h"
#include "Engine/Debug/Assert.h"
#include "Engine/Debug/Instrumentor.h"
#include "Engine/Renderer/GraphicsContext.h"
#include "Engine/Threads/Threads.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

static constexpr bool c_VerboseOpenGLDebugLog = false;

namespace eng
{
  static void openGLLogMessage(GLenum /*source*/, GLenum /*type*/, GLuint /*id*/, GLenum severity, GLsizei /*length*/, const GLchar* message, const void* /*userParam*/)
  {
    switch (severity)
    {
      case GL_DEBUG_SEVERITY_NOTIFICATION:  if (c_VerboseOpenGLDebugLog) ENG_CORE_TRACE(message); return;
      case GL_DEBUG_SEVERITY_LOW:           ENG_CORE_INFO(message);                               return;
      case GL_DEBUG_SEVERITY_MEDIUM:        ENG_CORE_WARN(message);                               return;
      case GL_DEBUG_SEVERITY_HIGH:          ENG_CORE_ERROR(message);                              return;
    }
    throw CoreException("OpenGL message has unknown severity level!");
  }

  OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
    : m_WindowHandle(windowHandle)
  {
    ENG_CORE_ASSERT(m_WindowHandle, "Window handle is null!");
  }

  void OpenGLContext::initialize()
  {
    ENG_PROFILE_FUNCTION();
    ENG_CORE_ASSERT(thread::isMainThread(), "OpenGL calls must be made on the main thread!");

    glfwMakeContextCurrent(m_WindowHandle);
    i32 status = gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress));
    if (!status)
      throw CoreException("Failed to initialize GLad!");

    ENG_CORE_INFO("OpenGL Info:");
    ENG_CORE_INFO("  Vendor:    {0}", std::bit_cast<const char*>(glGetString(GL_VENDOR)));
    ENG_CORE_INFO("  Renderer:  {0}", std::bit_cast<const char*>(glGetString(GL_RENDERER)));
    ENG_CORE_INFO("  Version:   {0}", std::bit_cast<const char*>(glGetString(GL_VERSION)));
    
    ENG_CORE_ASSERT(GLVersion.major > 4 || (GLVersion.major == 4 && GLVersion.minor >= 5), "Hazel requires at least OpenGL version 4.5!");

    #ifdef ENG_DEBUG
      // Initialize OpenGL debugging
      glEnable(GL_DEBUG_OUTPUT);
      glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
      glDebugMessageCallback(openGLLogMessage, nullptr);
    #endif

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Engine uses reversed-Z: https://nlguillemot.wordpress.com/2016/12/07/reversed-z-in-opengl/comment-page-1/
    glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
    glDepthFunc(GL_GREATER);
    glClearDepth(0.0f);
  }

  void OpenGLContext::swapBuffers()
  {
    ENG_PROFILE_FUNCTION();
    ENG_CORE_ASSERT(thread::isMainThread(), "OpenGL calls must be made on the main thread!");

    glfwSwapBuffers(m_WindowHandle);
  }
}