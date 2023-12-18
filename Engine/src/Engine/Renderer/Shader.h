#pragma once

namespace eng
{
  /*
    Abstract representation of a shader.
    Platform-specific implementation is determined by derived class.
  */
  class Shader
  {
  public:
    virtual ~Shader();

    virtual void bind() const = 0;
    virtual void unbind() const = 0;

    virtual const std::string& name() const = 0;

    static std::unique_ptr<Shader> Create(const std::filesystem::path& filepath, const std::unordered_map<std::string, std::string>& preprocessorDefinitions = {});

  protected:
    static std::string ReadFile(const std::filesystem::path& filepath);
    static std::unordered_map<std::string, std::string> PreProcess(std::string source, const std::unordered_map<std::string, std::string>& preprocessorDefinitions);
  };
}