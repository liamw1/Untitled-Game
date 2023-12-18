#pragma once
#include "Engine/Core/FixedWidthTypes.h"
#include "Engine/Renderer/Shader.h"

namespace eng
{
  class OpenGL_LegacyShader : public Shader
  {
    u32 m_ShaderID;
    std::string m_Name;

  public:
    OpenGL_LegacyShader(const std::filesystem::path& filepath, const std::unordered_map<std::string, std::string>& preprocessorDefinitions);
    ~OpenGL_LegacyShader();

    const std::string& name() const override;

    void bind() const override;
    void unbind() const override;

  private:
    void compile(const std::unordered_map<std::string, std::string>& shaderSources);
  };
}