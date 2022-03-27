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

    virtual void setInt(const std::string& name, int value) = 0;
    virtual void setIntArray(const std::string& name, int* values, uint32_t count) = 0;
    virtual void setFloat(const std::string& name, float value) = 0;
    virtual void setFloat2(const std::string& name, const Float2& values) = 0;
    virtual void setFloat3(const std::string& name, const Float3& values) = 0;
    virtual void setFloat4(const std::string& name, const Float4& values) = 0;
    virtual void setMat3(const std::string& name, const Float3x3& matrix) = 0;
    virtual void setMat4(const std::string& name, const Float4x4& matrix) = 0;

    virtual const std::string& getName() const = 0;

    static Unique<Shader> Create(const std::string& filepath);
    static Unique<Shader> Create(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource);
  };
}