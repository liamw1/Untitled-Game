#pragma once
#include "ENpch.h"
#include "Engine/Renderer/GraphicsContext.h"

struct GLFWwindow;

namespace Engine
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