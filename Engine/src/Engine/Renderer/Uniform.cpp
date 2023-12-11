#include "ENpch.h"
#include "Uniform.h"
#include "RendererAPI.h"
#include "Platform/OpenGL/OpenGLUniform.h"

namespace eng
{
  Uniform::~Uniform() = default;

  std::unique_ptr<Uniform> Uniform::Create(u32 binding, u32 size)
  {
    switch (RendererAPI::GetAPI())
    {
      case RendererAPI::API::OpenGL:        return std::make_unique<OpenGLUniform>(binding, size);
      case RendererAPI::API::OpenGL_Legacy: return std::make_unique<OpenGLUniform>(binding, size);
    }
    throw CoreException("Invalid RendererAPI!");
  }
}