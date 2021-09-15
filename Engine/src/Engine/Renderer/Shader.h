#pragma once

namespace Engine
{
  class Shader
  {
  public:
    virtual ~Shader() {}

    virtual void bind() const = 0;
    virtual void unBind() const = 0;

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