#include "ENpch.h"
#include "Shader.h"
#include "RendererAPI.h"
#include "Platform/OpenGL/OpenGLShader.h"
#include "Platform/OpenGL_Legacy/OpenGL_LegacyShader.h"

namespace Engine
{
  std::unique_ptr<Shader> Shader::Create(const std::string& filepath, const std::unordered_map<std::string, std::string>& preprocessorDefinitions)
  {
    switch (RendererAPI::GetAPI())
    {
      case RendererAPI::API::None:            EN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
      case RendererAPI::API::OpenGL:          return std::make_unique<OpenGLShader>(filepath, preprocessorDefinitions);
      case RendererAPI::API::OpenGL_Legacy:   return std::make_unique<OpenGL_LegacyShader>(filepath, preprocessorDefinitions);
      default:                                EN_CORE_ASSERT(false, "Unknown RendererAPI!"); return nullptr;
    }
  }
}