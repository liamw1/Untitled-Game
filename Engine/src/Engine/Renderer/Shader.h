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

    virtual void uploadUniformMat4(const glm::mat4& matrix, const std::string& name) = 0;

    static Shader* Create(const std::string& vertexSource, const std::string& fragmentSource);
  };
}