#include "ENpch.h"
#include "VertexArray.h"
#include "Renderer.h"
#include "Platform/OpenGL/OpenGLVertexArray.h"

namespace Engine
{
  Unique<IndexBuffer> IndexBuffer::Create(const uint32_t* indices, uint32_t count)
  {
    switch (Renderer::GetAPI())
    {
      case RendererAPI::API::None:    EN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
      case RendererAPI::API::OpenGL:  return CreateUnique<OpenGLIndexBuffer>(indices, count);
      default:                        EN_CORE_ASSERT(false, "Unknown RendererAPI!"); return nullptr;
    }
  }

  Unique<VertexArray> VertexArray::Create()
  {
    switch (Renderer::GetAPI())
    {
      case RendererAPI::API::None:    EN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
      case RendererAPI::API::OpenGL:  return CreateUnique<OpenGLVertexArray>();
      default:                        EN_CORE_ASSERT(false, "Unknown RendererAPI!"); return nullptr;
    }
  }
}