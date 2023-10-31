#pragma once
#include "Engine/Renderer/GraphicsContext.h"

struct GLFWwindow;

namespace eng
{
  class OpenGLContext : public GraphicsContext
  {
    GLFWwindow* m_WindowHandle;

  public:
    OpenGLContext(GLFWwindow* windowHandle);

    void initialize() override;
    void swapBuffers() override;
  };
}