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

    const std::string& getName() const override { return m_Name; }

    void bind() const override;
    void unBind() const override;

  private:
    uint32_t m_RendererID = 0;
    std::string m_Name;
    std::string m_FilePath;

    std::unordered_map<uint32_t, std::vector<uint32_t>> m_VulkanSPIRV;
    std::unordered_map<uint32_t, std::vector<uint32_t>> m_OpenGLSPIRV;
    std::unordered_map<uint32_t, std::string> m_OpenGLSourceCode;

    void compileOrGetVulkanBinaries(const std::unordered_map<std::string, std::string>& shaderSources);
    void compileOrGetOpenGLBinaries();
    void createProgram();
    void reflect(uint32_t stage, const std::vector<uint32_t>& shaderData);
  };
}