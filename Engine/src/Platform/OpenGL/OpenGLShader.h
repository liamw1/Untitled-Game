#pragma once
#include "Engine/Renderer/Shader.h"

namespace Engine
{
  class OpenGLShader : public Shader
  {
  public:
    OpenGLShader(const std::string& vertexSource, const std::string& fragmentSource);
    ~OpenGLShader();

    void bind() const override;
    void unBind() const override;

  private:
    uint32_t m_RendererID;
  };
}