#include "ENpch.h"
#include "VertexArray.h"
#include "RendererAPI.h"
#include "Platform/OpenGL/OpenGLVertexArray.h"

namespace eng
{
  IndexBuffer::IndexBuffer(const uint32_t* indices, uint32_t count)
    : m_Buffer(StorageBuffer::Create(StorageBuffer::Type::IndexBuffer))
  {
    m_Buffer->set(indices, count * sizeof(uint32_t));
  }
  IndexBuffer::IndexBuffer(const std::vector<uint32_t>& indices)
    : IndexBuffer(indices.data(), static_cast<uint32_t>(indices.size())) {}
  IndexBuffer::IndexBuffer(const std::shared_ptr<StorageBuffer>& indexBufferStorage)
    : m_Buffer(indexBufferStorage) {}

  void IndexBuffer::bind() const { m_Buffer->bind(); }
  void IndexBuffer::unBind() const { m_Buffer->unBind(); }

  uint32_t IndexBuffer::count() const
  {
    return m_Buffer->size() / sizeof(uint32_t);
  }



  VertexArray::~VertexArray() = default;

  std::unique_ptr<VertexArray> VertexArray::Create()
  {
    switch (RendererAPI::GetAPI())
    {
      case RendererAPI::API::OpenGL:        return std::make_unique<OpenGLVertexArray>();
      case RendererAPI::API::OpenGL_Legacy: return std::make_unique<OpenGLVertexArray>();
      default:                              throw  std::invalid_argument("Unknown RendererAPI!");
    }
  }

  void VertexArray::setVertexBuffer(const void* data, uint64_t size)
  {
    ENG_ASSERT(size < std::numeric_limits<uint32_t>::max(), "Requested vertex buffer size is greater than 32-bit unsigned integer limit!");
    setVertexBuffer(data, static_cast<uint32_t>(size));
  }
}