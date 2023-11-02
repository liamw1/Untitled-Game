#include "ENpch.h"
#include "Shader.h"
#include "RendererAPI.h"
#include "Engine/Debug/Instrumentor.h"
#include "Platform/OpenGL/OpenGLShader.h"
#include "Platform/OpenGL_Legacy/OpenGL_LegacyShader.h"

namespace eng
{
  Shader::~Shader() = default;

  std::unique_ptr<Shader> Shader::Create(const std::string& filepath, const std::unordered_map<std::string, std::string>& preprocessorDefinitions)
  {
    switch (RendererAPI::GetAPI())
    {
      case RendererAPI::API::OpenGL:          return std::make_unique<OpenGLShader>(filepath, preprocessorDefinitions);
      case RendererAPI::API::OpenGL_Legacy:   return std::make_unique<OpenGL_LegacyShader>(filepath, preprocessorDefinitions);
    }
    throw std::invalid_argument("Invalid RendererAPI!");
  }

  std::string Shader::ReadFile(const std::string& filepath)
  {
    ENG_PROFILE_FUNCTION();

    std::string result;
    std::ifstream in(filepath, std::ios::in | std::ios::binary);  // ifstream closes itself due to RAII
    if (in)
    {
      const uSize& size = in.tellg();
      if (size != -1)
      {
        in.seekg(0, std::ios::end);
        result.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&result[0], result.size());
      }
      else
        ENG_CORE_ERROR("Could not read from file {0}", filepath);
    }
    else
      ENG_CORE_ERROR("Could not open file {0}", filepath);

    return result;
  }

  std::unordered_map<std::string, std::string> Shader::PreProcess(std::string source, const std::unordered_map<std::string, std::string>& preprocessorDefinitions)
  {
    ENG_PROFILE_FUNCTION();

    uSize pos = 0;
    while ((pos = source.find("#define ", pos)) != std::string::npos)
    {
      pos += 8;

      uSize endToken = source.find('\n', pos) - 1;
      std::string token = source.substr(pos, endToken - pos);
      pos = endToken;

      std::unordered_map<std::string, std::string>::const_iterator iteratorPosition = preprocessorDefinitions.find(token);
      if (iteratorPosition != preprocessorDefinitions.end())
      {
        const auto& [key, definition] = *iteratorPosition;

        source.insert(pos, " " + definition);
        pos += definition.size();
      }
    }

    std::unordered_map<std::string, std::string> shaderSources;

    const char* typeToken = "#type";
    uSize typeTokenLength = strlen(typeToken);
    pos = source.find(typeToken, 0);                          // Start of shader type declaration line
    while (pos != std::string::npos)
    {
      uSize eol = source.find_first_of("\r\n", pos);    // End of shader type declaration line
      ENG_CORE_ASSERT(eol != std::string::npos, "Syntax error");
      uSize begin = pos + typeTokenLength + 1;          // Start of shader type name (after "#type" keyword)
      std::string type = source.substr(begin, eol - begin);

      uSize nextLinePos = source.find_first_not_of("\r\n", eol);
      ENG_CORE_ASSERT(nextLinePos != std::string::npos, "Syntax error");
      pos = source.find(typeToken, nextLinePos);              // Start of next shader type declaration line
      shaderSources[type] = (pos == std::string::npos) ? source.substr(nextLinePos) : source.substr(nextLinePos, pos - nextLinePos);
    }

    return shaderSources;
  }
}