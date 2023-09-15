#include "ENpch.h"
#include "Uniform.h"
#include "RendererAPI.h"
#include "Platform/OpenGL/OpenGLUniform.h"

namespace Engine
{
  Uniform::~Uniform() = default;

  std::unique_ptr<Uniform> Uniform::Create(uint32_t binding, uint32_t size)
  {
    switch (RendererAPI::GetAPI())
    {
      case RendererAPI::API::OpenGL:        return std::make_unique<OpenGLUniform>(binding, size);
      case RendererAPI::API::OpenGL_Legacy: return std::make_unique<OpenGLUniform>(binding, size);
      default:                              throw  std::invalid_argument("Unknown RendererAPI!");
    }
  }
}