#pragma once

namespace Engine
{
  class Shader
  {
  public:
    virtual ~Shader() {}

    virtual void bind() const = 0;
    virtual void unBind() const = 0;

    static Shader* Create(const std::string& vertexSource, const std::string& fragmentSource);
  };
}