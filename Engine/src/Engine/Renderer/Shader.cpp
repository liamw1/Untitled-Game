#include "ENpch.h"
#include "Shader.h"
#include "RendererAPI.h"
#include "Platform/OpenGL/OpenGLShader.h"
#include "Platform/OpenGL_Legacy/OpenGL_LegacyShader.h"

namespace Engine
{
  std::unique_ptr<Shader> Shader::Create(const std::string& filepath)
  {
    switch (RendererAPI::GetAPI())
    {
      case RendererAPI::API::None:            EN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
      case RendererAPI::API::OpenGL:          return std::make_unique<OpenGLShader>(filepath);
      case RendererAPI::API::OpenGL_Legacy:   return std::make_unique<OpenGL_LegacyShader>(filepath);
      default:                                EN_CORE_ASSERT(false, "Unknown RendererAPI!"); return nullptr;
    }
  }
}