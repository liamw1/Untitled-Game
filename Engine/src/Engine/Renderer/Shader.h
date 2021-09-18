#pragma once
#include <glm/glm.hpp>

namespace Engine
{
  class Shader
  {
  public:
    virtual ~Shader() {}

    virtual void bind() const = 0;
    virtual void unBind() const = 0;

    virtual void setInt(const std::string& name, int value) = 0;
    virtual void setFloat(const std::string& name, float value) = 0;
    virtual void setFloat2(const std::string& name, const glm::vec2& values) = 0;
    virtual void setFloat3(const std::string& name, const glm::vec3& values) = 0;
    virtual void setFloat4(const std::string& name, const glm::vec4& values) = 0;
    virtual void setMat3(const std::string& name, const glm::mat3& matrix) = 0;
    virtual void setMat4(const std::string& name, const glm::mat4& matrix) = 0;

    virtual const std::string& getName() const = 0;

    static Shared<Shader> Create(const std::string& filepath);
    static Shared<Shader> Create(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource);
  };



  class ShaderLibrary
  {
  public:
    void add(const Shared<Shader>& shader);
    void add(const std::string& name, const Shared<Shader>& shader);
    Shared<Shader> load(const std::string& filepath);
    Shared<Shader> load(const std::string& name, const std::string& filepath);

    Shared<Shader> get(const std::string& name);

  private:
    std::unordered_map<std::string, Shared<Shader>> m_Shaders;
  };
}