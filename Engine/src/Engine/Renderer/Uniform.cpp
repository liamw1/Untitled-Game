#include "ENpch.h"
#include "Uniform.h"
#include "RendererAPI.h"
#include "Platform/OpenGL/OpenGLUniform.h"

namespace Engine
{
  std::unique_ptr<Uniform> Uniform::Create(uint32_t binding, uint32_t size)
  {
    switch (RendererAPI::GetAPI())
    {
      case RendererAPI::API::None:          EN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
      case RendererAPI::API::OpenGL:        return std::make_unique<OpenGLUniform>(binding, size);
      case RendererAPI::API::OpenGL_Legacy: return std::make_unique<OpenGLUniform>(binding, size);
      default:                              EN_CORE_ASSERT(false, "Unknown RendererAPI!"); return nullptr;
    }
  }
}