#include "ENpch.h"
#include "DynamicBuffer.h"
#include "Engine/Renderer/RendererAPI.h"
#include "Platform/OpenGL/OpenGLDynamicBuffer.h"

namespace eng::mem
{
  DynamicBuffer::~DynamicBuffer() = default;
  std::unique_ptr<DynamicBuffer> DynamicBuffer::Create(Type type)
  {
    switch (RendererAPI::GetAPI())
    {
      case RendererAPI::API::OpenGL:          return std::make_unique<OpenGLDynamicBuffer>(type);
      case RendererAPI::API::OpenGL_Legacy:   return std::make_unique<OpenGLDynamicBuffer>(type);
    }
    throw CoreException("Invalid RendererAPI!");
  }
}