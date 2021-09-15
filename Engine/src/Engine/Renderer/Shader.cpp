#include "ENpch.h"
#include "Shader.h"
#include "Renderer.h"
#include "Platform/OpenGL/OpenGLShader.h"

namespace Engine
{
  Shared<Shader> Shader::Create(const std::string& vertexSource, const std::string& fragmentSource)
  {
    switch (Renderer::GetAPI())
    {
      case RendererAPI::API::None:    EN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
      case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLShader>(vertexSource, fragmentSource);
      default:                        EN_CORE_ASSERT(false, "Unknown RendererAPI!"); return nullptr;
    }
  }
}