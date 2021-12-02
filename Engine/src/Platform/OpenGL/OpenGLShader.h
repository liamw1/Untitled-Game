#pragma once
#include "Engine/Renderer/Shader.h"
#include <glm/glm.hpp>

// TEMPORARY
typedef unsigned int GLenum;
typedef int GLint;

namespace Engine
{
  class OpenGLShader : public Shader
  {
  public:
    OpenGLShader(const std::string& filepath);
    OpenGLShader(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource);
    ~OpenGLShader();

    const std::string& getName() const override { return m_Name; }

    void bind() const override;
    void unBind() const override;

    void setInt(const std::string& name, int value) override;
    void setIntArray(const std::string& name, int* values, uint32_t count) override;
    void setFloat(const std::string& name, float value) override;
    void setFloat2(const std::string& name, const Float2& values) override;
    void setFloat3(const std::string& name, const Float3& values) override;
    void setFloat4(const std::string& name, const Float4& values) override;
    void setMat3(const std::string& name, const Float3x3& matrix) override;
    void setMat4(const std::string& name, const Float4x4& matrix) override;

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