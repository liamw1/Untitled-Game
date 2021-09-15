#pragma once
#include "Engine/Renderer/Shader.h"
#include <glm/glm.hpp>

// TEMPORARY
typedef unsigned int GLenum;

namespace Engine
{
  class OpenGLShader : public Shader
  {
  public:
    OpenGLShader(const std::string& filepath);
    OpenGLShader(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource);
    ~OpenGLShader();

    inline const std::string& getName() const override { return m_Name; }

    void bind() const override;
    void unBind() const override;

    void uploadUniformInt(const std::string& name, int value);

    void uploadUniformFloat(const std::string& name, const float value);
    void uploadUniformFloat2(const std::string& name, const glm::vec2& values);
    void uploadUniformFloat3(const std::string& name, const glm::vec3& values);
    void uploadUniformFloat4(const std::string& name, const glm::vec4& values);

    void uploadUniformMat3(const std::string& name, const glm::mat3& matrix);
    void uploadUniformMat4(const std::string& name, const glm::mat4& matrix);

  private:
    uint32_t m_RendererID = 0;
    std::string m_Name;

    std::string readFile(const std::string& filepath);
    std::unordered_map<GLenum, std::string> preProcess(const std::string& source);
    void compile(const std::unordered_map<GLenum, std::string>& shaderSources);
  };
}