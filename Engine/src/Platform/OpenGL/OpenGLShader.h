#pragma once
#include "Engine/Renderer/Shader.h"

// TODO: Remove
typedef unsigned int GLenum;
typedef int GLint;

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
    mutable std::unordered_map<std::string, GLint> m_UniformLocationCache;

    std::unordered_map<GLenum, std::vector<uint32_t>> m_VulkanSPIRV;
    std::unordered_map<GLenum, std::vector<uint32_t>> m_OpenGLSPIRV;
    std::unordered_map<GLenum, std::string> m_OpenGLSourceCode;

    std::string readFile(const std::string& filepath);
    std::unordered_map<GLenum, std::string> preProcess(const std::string& source);

    void compileOrGetVulkanBinaries(const std::unordered_map<GLenum, std::string>& shaderSources);
    void compileOrGetOpenGLBinaries();
    void createProgram();
    void reflect(GLenum stage, const std::vector<uint32_t>& shaderData);

    /*
      NOTE: Temporary function for retrieving uniform locations more
      quickly than querying openGL.  The proper fix would be to parse
      shaders to find and store all uniform locations at compilation.
    */
    GLint getUniformLocation(const std::string& name) const;
  };
}