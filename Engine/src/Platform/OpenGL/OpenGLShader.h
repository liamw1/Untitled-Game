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

    void uploadUniformMat4(const glm::mat4& matrix, const std::string& name) override;

  private:
    uint32_t m_RendererID;
  };
}