#pragma once
#include "Engine/Renderer/GraphicsContext.h"

struct GLFWwindow;

namespace eng
{
  class OpenGLContext : public GraphicsContext
  {
  public:
    OpenGLContext(GLFWwindow* windowHandle);

    void initialize() override;
    void swapBuffers() override;

  private:
    GLFWwindow* m_WindowHandle;
  };
}