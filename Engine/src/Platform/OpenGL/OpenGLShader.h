#pragma once
#include "Engine/Core/FixedWidthTypes.h"
#include "Engine/Renderer/Shader.h"

namespace eng
{
  /*
    NOTE: Currently, shader system caches shader compilation but does not
          check if shader source code has been modified.  This means that the
          cache directory must be deleted to recompile shaders.  In the future,
          shader sources should be hashed and recompiled if necessary.  TODO
  */
  class OpenGLShader : public Shader
  {
    u32 m_ShaderID;
    std::string m_Name;
    std::string m_FilePath;

    std::unordered_map<u32, std::vector<u32>> m_VulkanSPIRV;
    std::unordered_map<u32, std::vector<u32>> m_OpenGLSPIRV;
    std::unordered_map<u32, std::string> m_OpenGLSourceCode;

  public:
    OpenGLShader(const std::filesystem::path& filepath, const std::unordered_map<std::string, std::string>& preprocessorDefinitions);
    ~OpenGLShader();

    std::string_view name() const override;

    void bind() const override;
    void unbind() const override;

  private:
    void compileVulkanBinaries(const std::unordered_map<std::string, std::string>& shaderSources);
    void compileOpenGLBinaries();
    void createProgram();
    void reflect(u32 stage, const std::vector<u32>& shaderData);
  };
}