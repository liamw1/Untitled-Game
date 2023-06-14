#pragma once

namespace Engine
{
  /*
    Abstract representation of a shader.
    Platform-specific implementation is determined by derived class.
  */
  class Shader
  {
  public:
    virtual ~Shader() = default;

    virtual void bind() const = 0;
    virtual void unBind() const = 0;

    virtual const std::string& getName() const = 0;

    static std::unique_ptr<Shader> Create(const std::string& filepath);
    static std::unique_ptr<Shader> Create(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource);
  };
}