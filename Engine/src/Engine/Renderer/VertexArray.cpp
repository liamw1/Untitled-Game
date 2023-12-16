#include "ENpch.h"
#include "VertexArray.h"
#include "RendererAPI.h"
#include "Platform/OpenGL/OpenGLVertexArray.h"

namespace eng
{
  IndexBuffer::IndexBuffer(const mem::IndexData& data)
    : m_Buffer(mem::DynamicBuffer::Create(mem::DynamicBuffer::Type::Index))
  {
    m_Buffer->set(static_cast<mem::RenderData>(data));
  }
  IndexBuffer::IndexBuffer(const std::shared_ptr<mem::DynamicBuffer>& indexBuffer)
    : m_Buffer(indexBuffer) {}

  void IndexBuffer::bind() const { m_Buffer->bind(); }
  void IndexBuffer::unbind() const { m_Buffer->unbind(); }

  uSize IndexBuffer::count() const
  {
    return m_Buffer->size() / sizeof(u32);
  }



  VertexArray::~VertexArray() = default;

  std::unique_ptr<VertexArray> VertexArray::Create()
  {
    switch (RendererAPI::GetAPI())
    {
      case RendererAPI::API::OpenGL:        return std::make_unique<OpenGLVertexArray>();
      case RendererAPI::API::OpenGL_Legacy: return std::make_unique<OpenGLVertexArray>();
    }
    throw CoreException("Invalid RendererAPI!");
  }
}