#include "ENpch.h"
#include "OpenGLVertexArray.h"
#include "Engine/Core/Casting.h"
#include "Engine/Debug/Assert.h"
#include "Engine/Threads/Threads.h"

#include <glad/glad.h>

namespace eng
{
  static GLenum convertToOpenGLBaseType(mem::ShaderDataType type)
  {
    switch (type)
    {
      case mem::ShaderDataType::Bool:        return GL_BOOL;
      case mem::ShaderDataType::Uint32:      return GL_UNSIGNED_INT;
      case mem::ShaderDataType::Int:         return GL_INT;
      case mem::ShaderDataType::Int2:        return GL_INT;
      case mem::ShaderDataType::Int3:        return GL_INT;
      case mem::ShaderDataType::Int4:        return GL_INT;
      case mem::ShaderDataType::Float:       return GL_FLOAT;
      case mem::ShaderDataType::Float2:      return GL_FLOAT;
      case mem::ShaderDataType::Float3:      return GL_FLOAT;
      case mem::ShaderDataType::Float4:      return GL_FLOAT;
      case mem::ShaderDataType::Mat3:        return GL_FLOAT;
      case mem::ShaderDataType::Mat4:        return GL_FLOAT;
    }
    throw std::invalid_argument("Invalid ShaderDataType!");
  }

  OpenGLVertexArray::OpenGLVertexArray()
  {
    ENG_CORE_ASSERT(thread::isMainThread(), "OpenGL calls must be made on the main thread!");

    glCreateVertexArrays(1, &m_RendererID);
    m_VertexBuffer = mem::StorageBuffer::Create(mem::StorageBuffer::Type::VertexBuffer);
  }

  OpenGLVertexArray::~OpenGLVertexArray()
  {
    ENG_CORE_ASSERT(thread::isMainThread(), "OpenGL calls must be made on the main thread!");
    glDeleteVertexArrays(1, &m_RendererID);
  }

  void OpenGLVertexArray::bind() const
  {
    ENG_CORE_ASSERT(thread::isMainThread(), "OpenGL calls must be made on the main thread!");

    glBindVertexArray(m_RendererID);

    if (m_IndexBuffer)
      m_IndexBuffer->bind();
  }

  void OpenGLVertexArray::unBind() const
  {
    ENG_CORE_ASSERT(thread::isMainThread(), "OpenGL calls must be made on the main thread!");

    glBindVertexArray(0);

    if (m_VertexBuffer)
      m_VertexBuffer->unBind();
    if (m_IndexBuffer)
      m_IndexBuffer->unBind();
  }

  void OpenGLVertexArray::setLayout(const mem::BufferLayout& layout)
  {
    ENG_CORE_ASSERT(thread::isMainThread(), "OpenGL calls must be made on the main thread!");

    m_VertexBufferLayout = layout;

    glBindVertexArray(m_RendererID);
    m_VertexBuffer->bind();

    u32 vertexBufferIndex = 0;
    for (const mem::BufferElement& element : layout)
    {
      std::underlying_type_t<mem::ShaderDataType> dataTypeID = toUnderlying(element.type);

      if (dataTypeID >= toUnderlying(mem::ShaderDataType::FloatTypeBegin) && dataTypeID <= toUnderlying(mem::ShaderDataType::FloatTypeEnd))
      {
        glEnableVertexAttribArray(vertexBufferIndex);
        glVertexAttribPointer(vertexBufferIndex,
          element.getComponentCount(),
          convertToOpenGLBaseType(element.type),
          element.normalized ? GL_TRUE : GL_FALSE,
          layout.stride(),
          std::bit_cast<const void*>(arithmeticUpcast<uSize>(element.offset)));
        vertexBufferIndex++;
      }
      else if (dataTypeID >= toUnderlying(mem::ShaderDataType::IntTypeBegin) && dataTypeID <= toUnderlying(mem::ShaderDataType::IntTypeEnd))
      {
        glEnableVertexAttribArray(vertexBufferIndex);
        glVertexAttribIPointer(vertexBufferIndex,
          element.getComponentCount(),
          convertToOpenGLBaseType(element.type),
          layout.stride(),
          std::bit_cast<const void*>(arithmeticUpcast<uSize>(element.offset)));
        vertexBufferIndex++;
      }
      else if (dataTypeID >= toUnderlying(mem::ShaderDataType::MatTypeBegin) && dataTypeID <= toUnderlying(mem::ShaderDataType::MatTypeEnd))
      {
        i32 componentCount = element.getComponentCount();
        for (i32 i = 0; i < componentCount; ++i)
        {
          glEnableVertexAttribArray(vertexBufferIndex);
          glVertexAttribPointer(vertexBufferIndex,
                                componentCount,
                                convertToOpenGLBaseType(element.type),
                                element.normalized ? GL_TRUE : GL_FALSE,
                                layout.stride(),
                                std::bit_cast<const void*>(arithmeticUpcast<uSize>(element.offset + sizeof(f32) * componentCount * i)));
          glVertexAttribDivisor(vertexBufferIndex, 1);
          vertexBufferIndex++;
        }
      }
      else
        throw std::invalid_argument("Invalid shader data type!");
    }

#if ENG_DEBUG
    unBind();
#endif
  }

  void OpenGLVertexArray::setVertexBuffer(const mem::Data& data)
  {
    m_VertexBuffer->set(data);

#if ENG_DEBUG
    unBind();
#endif
  }

  void OpenGLVertexArray::setVertexBuffer(const std::shared_ptr<mem::StorageBuffer>& vertexBuffer)
  {
    m_VertexBuffer = vertexBuffer;
    setLayout(m_VertexBufferLayout);
  }

  void OpenGLVertexArray::modifyVertexBuffer(u32 offset, const mem::Data& data) const
  {
    m_VertexBuffer->modify(offset, data);

#if ENG_DEBUG
    unBind();
#endif
  }

  void OpenGLVertexArray::resizeVertexBuffer(u32 newSize)
  {
    m_VertexBuffer->resize(newSize);
    setLayout(m_VertexBufferLayout);

#if ENG_DEBUG
    unBind();
#endif
  }

  void OpenGLVertexArray::setIndexBuffer(const IndexBuffer& indexBuffer)
  {
    ENG_CORE_ASSERT(thread::isMainThread(), "OpenGL calls must be made on the main thread!");

    m_IndexBuffer = indexBuffer;

#if ENG_DEBUG
    unBind();
#endif
  }

  void OpenGLVertexArray::setIndexBuffer(const std::shared_ptr<mem::StorageBuffer>& indexBufferStorage)
  {
    ENG_CORE_ASSERT(thread::isMainThread(), "OpenGL calls must be made on the main thread!");
    ENG_CORE_ASSERT(indexBufferStorage->type() == StorageBuffer::Type::IndexBuffer, "Submitted buffer is not an index buffer!");

    m_IndexBuffer = IndexBuffer(indexBufferStorage);

#if ENG_DEBUG
    unBind();
#endif
  }

  const mem::BufferLayout& OpenGLVertexArray::getLayout() const
  {
    return m_VertexBufferLayout;
  }

  const std::optional<IndexBuffer>& OpenGLVertexArray::getIndexBuffer() const
  {
    return m_IndexBuffer;
  }
}