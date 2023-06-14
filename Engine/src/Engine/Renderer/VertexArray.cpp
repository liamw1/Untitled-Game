#include "ENpch.h"
#include "VertexArray.h"
#include "RendererAPI.h"
#include "Platform/OpenGL/OpenGLVertexArray.h"

namespace Engine
{
  std::unique_ptr<IndexBuffer> IndexBuffer::Create(const uint32_t* indices, uint32_t count)
  {
    switch (RendererAPI::GetAPI())
    {
      case RendererAPI::API::None:            EN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
      case RendererAPI::API::OpenGL:          return std::make_unique<OpenGLIndexBuffer>(indices, count);
      case RendererAPI::API::OpenGL_Legacy:   return std::make_unique<OpenGLIndexBuffer>(indices, count);
      default:                                EN_CORE_ASSERT(false, "Unknown RendererAPI!"); return nullptr;
    }
  }

  std::unique_ptr<IndexBuffer> IndexBuffer::Create(const std::vector<uint32_t>& indices)
  {
    return IndexBuffer::Create(indices.data(), static_cast<uint32_t>(indices.size()));
  }



  std::unique_ptr<VertexArray> VertexArray::Create()
  {
    switch (RendererAPI::GetAPI())
    {
      case RendererAPI::API::None:          EN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
      case RendererAPI::API::OpenGL:        return std::make_unique<OpenGLVertexArray>();
      case RendererAPI::API::OpenGL_Legacy: return std::make_unique<OpenGLVertexArray>();
      default:                              EN_CORE_ASSERT(false, "Unknown RendererAPI!"); return nullptr;
    }
  }
}