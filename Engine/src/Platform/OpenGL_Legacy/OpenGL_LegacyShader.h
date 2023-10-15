#pragma once
#include "Engine/Renderer/Shader.h"

namespace eng
{
  class OpenGL_LegacyShader : public Shader
  {
  public:
    OpenGL_LegacyShader(const std::string& filepath, const std::unordered_map<std::string, std::string>& preprocessorDefinitions);
    ~OpenGL_LegacyShader();

    const std::string& name() const override;

    void bind() const override;
    void unBind() const override;

  private:
    uint32_t m_RendererID;
    std::string m_Name;

    void compile(const std::unordered_map<std::string, std::string>& shaderSources);
  };
}