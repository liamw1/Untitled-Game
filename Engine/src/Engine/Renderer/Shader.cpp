#include "ENpch.h"
#include "Shader.h"
#include "RendererAPI.h"
#include "Engine/Debug/Instrumentor.h"
#include "Platform/OpenGL/OpenGLShader.h"
#include "Platform/OpenGL_Legacy/OpenGL_LegacyShader.h"

namespace eng
{
  Shader::~Shader() = default;

  std::unique_ptr<Shader> Shader::Create(const std::filesystem::path& filepath, const std::unordered_map<std::string, std::string>& preprocessorDefinitions)
  {
    switch (RendererAPI::GetAPI())
    {
      case RendererAPI::API::OpenGL:          return std::make_unique<OpenGLShader>(filepath, preprocessorDefinitions);
      case RendererAPI::API::OpenGL_Legacy:   return std::make_unique<OpenGL_LegacyShader>(filepath, preprocessorDefinitions);
    }
    throw CoreException("Invalid RendererAPI!");
  }

  std::string Shader::ReadFile(const std::filesystem::path& filepath)
  {
    ENG_PROFILE_FUNCTION();

    std::stringstream ss;
    std::ifstream fileStream(filepath, std::ios::in | std::ios::binary);
    if (fileStream && fileStream.tellg() != -1)
      ss << fileStream.rdbuf();
    else
      ENG_CORE_ERROR("Unable to read from file {0}", filepath);

    return ss.str();
  }

  std::unordered_map<std::string, std::string> Shader::PreProcess(std::string source, const std::unordered_map<std::string, std::string>& preprocessorDefinitions)
  {
    ENG_PROFILE_FUNCTION();

    static constexpr std::string_view defineToken = "#define";
    static constexpr std::string_view typeToken = "#type";

    uSize sourcePosition = 0;
    while ((sourcePosition = source.find(defineToken, sourcePosition)) != std::string::npos)
    {
      sourcePosition += defineToken.size() + 1;

      uSize endTokenPosition = source.find('\n', sourcePosition) - 1;
      std::string token = source.substr(sourcePosition, endTokenPosition - sourcePosition);
      sourcePosition = endTokenPosition;

      std::unordered_map<std::string, std::string>::const_iterator iteratorPosition = preprocessorDefinitions.find(token);
      if (iteratorPosition == preprocessorDefinitions.end())
        continue;

      const std::string& definition = iteratorPosition->second;
      source.insert(sourcePosition, " " + definition);
      sourcePosition += definition.size();
    }

    std::unordered_map<std::string, std::string> shaderSources;

    sourcePosition = source.find(typeToken, 0);                           // Start of shader type declaration line
    while (sourcePosition != std::string::npos)
    {
      uSize endOfLine = source.find_first_of("\r\n", sourcePosition);     // End of shader type declaration line
      ENG_CORE_ASSERT(endOfLine != std::string::npos, "Syntax error");
      uSize typeBegin = sourcePosition + typeToken.size() + 1;            // Start of shader type name (after "#type" keyword)
      std::string type = source.substr(typeBegin, endOfLine - typeBegin);

      uSize nextLinePos = source.find_first_not_of("\r\n", endOfLine);
      ENG_CORE_ASSERT(nextLinePos != std::string::npos, "Syntax error");
      sourcePosition = source.find(typeToken, nextLinePos);               // Start of next shader type declaration line
      shaderSources[type] = source.substr(nextLinePos, sourcePosition - nextLinePos);
    }

    return shaderSources;
  }
}