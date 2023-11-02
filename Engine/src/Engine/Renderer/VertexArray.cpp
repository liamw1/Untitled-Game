#include "ENpch.h"
#include "VertexArray.h"
#include "RendererAPI.h"
#include "Platform/OpenGL/OpenGLVertexArray.h"

namespace eng
{
  IndexBuffer::IndexBuffer(const u32* indices, u32 count)
    : m_Buffer(StorageBuffer::Create(StorageBuffer::Type::IndexBuffer))
  {
    m_Buffer->set(indices, count * sizeof(u32));
  }
  IndexBuffer::IndexBuffer(const std::vector<u32>& indices)
    : IndexBuffer(indices.data(), arithmeticCast<u32>(indices.size())) {}
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

  void VertexArray::setVertexBuffer(const void* data, u64 size)
  {
    setVertexBuffer(data, arithmeticCast<u32>(size));
  }
}