#include "ENpch.h"
#include "VertexArray.h"
#include "RendererAPI.h"
#include "Platform/OpenGL/OpenGLVertexArray.h"

namespace eng
{
  IndexBuffer::IndexBuffer(const mem::IndexData& data)
    : m_Buffer(StorageBuffer::Create(StorageBuffer::Type::IndexBuffer))
  {
    m_Buffer->set(static_cast<mem::Data>(data));
  }
  IndexBuffer::IndexBuffer(const std::shared_ptr<StorageBuffer>& indexBufferStorage)
    : m_Buffer(indexBufferStorage) {}

  void IndexBuffer::bind() const { m_Buffer->bind(); }
  void IndexBuffer::unBind() const { m_Buffer->unBind(); }

  u32 IndexBuffer::count() const
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
    throw std::invalid_argument("Invalid RendererAPI!");
  }
}