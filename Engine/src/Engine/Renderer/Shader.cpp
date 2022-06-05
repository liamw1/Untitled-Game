#include "ENpch.h"
#include "Shader.h"
#include "RendererAPI.h"
#include "Platform/OpenGL/OpenGLShader.h"
#include "Platform/OpenGL_Legacy/OpenGL_LegacyShader.h"

namespace Engine
{
  Unique<Shader> Shader::Create(const std::string& filepath)
  {
    switch (RendererAPI::GetAPI())
    {
      case RendererAPI::API::None:            EN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
      case RendererAPI::API::OpenGL:          return CreateUnique<OpenGLShader>(filepath);
      case RendererAPI::API::OpenGL_Legacy:   return CreateUnique<OpenGL_LegacyShader>(filepath);
      default:                                EN_CORE_ASSERT(false, "Unknown RendererAPI!"); return nullptr;
    }
  }
  Unique<Shader> Shader::Create(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource)
  {
    switch (RendererAPI::GetAPI())
    {
      case RendererAPI::API::None:            EN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
      case RendererAPI::API::OpenGL:          return CreateUnique<OpenGLShader>(name, vertexSource, fragmentSource);
      case RendererAPI::API::OpenGL_Legacy:   return CreateUnique<OpenGL_LegacyShader>(name, vertexSource, fragmentSource);
      default:                                EN_CORE_ASSERT(false, "Unknown RendererAPI!"); return nullptr;
    }
  }
}