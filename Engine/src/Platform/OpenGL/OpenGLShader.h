#pragma once
#include "Engine/Renderer/Shader.h"

namespace Engine
{
  /*
    NOTE: Currently, shader system caches shader compilation but does not
          check if shader source code has been modified.  This means that the
          cache directory must be deleted to recompile shaders.  In the future,
          shader sources should be hashed and recompiled if necessary.  TODO
  */
  class OpenGLShader : public Shader
  {
  public:
    OpenGLShader(const std::string& filepath, const std::unordered_map<std::string, std::string>& preprocessorDefinitions);
    ~OpenGLShader();

    const std::string& name() const override;

    void bind() const override;
    void unBind() const override;

  private:
    uint32_t m_RendererID;
    std::string m_Name;
    std::string m_FilePath;

    std::unordered_map<uint32_t, std::vector<uint32_t>> m_VulkanSPIRV;
    std::unordered_map<uint32_t, std::vector<uint32_t>> m_OpenGLSPIRV;
    std::unordered_map<uint32_t, std::string> m_OpenGLSourceCode;

    void compileVulkanBinaries(const std::unordered_map<std::string, std::string>& shaderSources);
    void compileOpenGLBinaries();
    void createProgram();
    void reflect(uint32_t stage, const std::vector<uint32_t>& shaderData);
  };
}