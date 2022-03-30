#pragma once
#include "Engine/Renderer/Shader.h"

// TODO: Remove
typedef unsigned int GLenum;
typedef int GLint;

namespace Engine
{
  class OpenGL_LegacyShader : public Shader
  {
  public:
    OpenGL_LegacyShader(const std::string& filepath);
    OpenGL_LegacyShader(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource);
    ~OpenGL_LegacyShader();

    const std::string& getName() const override { return m_Name; }

    void bind() const override;
    void unBind() const override;

  private:
    uint32_t m_RendererID = 0;
    std::string m_Name;
    mutable std::unordered_map<std::string, GLint> m_UniformLocationCache;

    std::string readFile(const std::string& filepath);
    std::unordered_map<GLenum, std::string> preProcess(const std::string& source);
    void compile(const std::unordered_map<GLenum, std::string>& shaderSources);

    /*
      NOTE: Temporary function for retrieving uniform locations more
      quickly than querying openGL.  The proper fix would be to parse
      shaders to find and store all uniform locations at compilation.
    */
    GLint getUniformLocation(const std::string& name) const;
  };
}