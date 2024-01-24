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

    virtual std::string_view name() const = 0;

    static std::unique_ptr<Shader> Create(const std::filesystem::path& filepath, const std::unordered_map<std::string, std::string>& preprocessorDefinitions = {});

  protected:
    static std::string ReadFile(const std::filesystem::path& filepath);

    // Passing source by value is intentional
    static std::unordered_map<std::string, std::string> PreProcess(std::string source, const std::unordered_map<std::string, std::string>& preprocessorDefinitions);
  };
}