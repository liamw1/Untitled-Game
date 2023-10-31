#include "ENpch.h"
#include "OpenGLVertexArray.h"
#include "Engine/Threads/Threads.h"
#include <glad/glad.h>

namespace eng
{
  static GLenum convertToOpenGLBaseType(ShaderDataType type)
  {
    switch (type)
    {
      case ShaderDataType::Bool:        return GL_BOOL;
      case ShaderDataType::Uint32:      return GL_UNSIGNED_INT;
      case ShaderDataType::Int:         return GL_INT;
      case ShaderDataType::Int2:        return GL_INT;
      case ShaderDataType::Int3:        return GL_INT;
      case ShaderDataType::Int4:        return GL_INT;
      case ShaderDataType::Float:       return GL_FLOAT;
      case ShaderDataType::Float2:      return GL_FLOAT;
      case ShaderDataType::Float3:      return GL_FLOAT;
      case ShaderDataType::Float4:      return GL_FLOAT;
      case ShaderDataType::Mat3:        return GL_FLOAT;
      case ShaderDataType::Mat4:        return GL_FLOAT;
      default: ENG_CORE_ASSERT(false, "Unknown ShaderDataType!"); return 0;
    }
  }

  OpenGLVertexArray::OpenGLVertexArray()
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");

    glCreateVertexArrays(1, &m_RendererID);
    m_VertexBuffer = StorageBuffer::Create(StorageBuffer::Type::VertexBuffer);
  }

  OpenGLVertexArray::~OpenGLVertexArray()
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");
    glDeleteVertexArrays(1, &m_RendererID);
  }

  void OpenGLVertexArray::bind() const
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");

    glBindVertexArray(m_RendererID);

    if (m_IndexBuffer)
      m_IndexBuffer->bind();
  }

  void OpenGLVertexArray::unBind() const
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");

    glBindVertexArray(0);

    if (m_VertexBuffer)
      m_VertexBuffer->unBind();
    if (m_IndexBuffer)
      m_IndexBuffer->unBind();
  }

  void OpenGLVertexArray::setLayout(const BufferLayout& layout)
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");

    m_VertexBufferLayout = layout;

    glBindVertexArray(m_RendererID);
    m_VertexBuffer->bind();

    u32 vertexBufferIndex = 0;
    for (const BufferElement& element : layout)
    {
      const i32 dataTypeID = static_cast<i32>(element.type);

      if (dataTypeID >= static_cast<i32>(ShaderDataType::FloatTypeBegin) && dataTypeID <= static_cast<i32>(ShaderDataType::FloatTypeEnd))
      {
        glEnableVertexAttribArray(vertexBufferIndex);
        glVertexAttribPointer(vertexBufferIndex,
          element.getComponentCount(),
          convertToOpenGLBaseType(element.type),
          element.normalized ? GL_TRUE : GL_FALSE,
          layout.stride(),
          std::bit_cast<const void*>(static_cast<uSize>(element.offset)));
        vertexBufferIndex++;
      }
      else if (dataTypeID >= static_cast<i32>(ShaderDataType::IntTypeBegin) && dataTypeID <= static_cast<i32>(ShaderDataType::IntTypeEnd))
      {
        glEnableVertexAttribArray(vertexBufferIndex);
        glVertexAttribIPointer(vertexBufferIndex,
          element.getComponentCount(),
          convertToOpenGLBaseType(element.type),
          layout.stride(),
          std::bit_cast<const void*>(static_cast<uSize>(element.offset)));
        vertexBufferIndex++;
      }
      else if (dataTypeID >= static_cast<i32>(ShaderDataType::MatTypeBegin) && dataTypeID <= static_cast<i32>(ShaderDataType::MatTypeEnd))
      {
        i32 count = element.getComponentCount();
        for (i32 i = 0; i < count; ++i)
        {
          glEnableVertexAttribArray(vertexBufferIndex);
          glVertexAttribPointer(vertexBufferIndex,
            count,
            convertToOpenGLBaseType(element.type),
            element.normalized ? GL_TRUE : GL_FALSE,
            layout.stride(),
            std::bit_cast<const void*>(static_cast<uSize>(element.offset + sizeof(f32) * count * i)));
          glVertexAttribDivisor(vertexBufferIndex, 1);
          vertexBufferIndex++;
        }
      }
      else
        ENG_CORE_ASSERT(false, ("Unknown shader data type!"));
    }

#if ENG_DEBUG
    unBind();
#endif
  }

  void OpenGLVertexArray::setVertexBuffer(const void* data, u32 size)
  {
    m_VertexBuffer->set(data, size);

#if ENG_DEBUG
    unBind();
#endif
  }

  void OpenGLVertexArray::setVertexBuffer(const std::shared_ptr<StorageBuffer>& vertexBuffer)
  {
    m_VertexBuffer = vertexBuffer;
    setLayout(m_VertexBufferLayout);
  }

  void OpenGLVertexArray::updateVertexBuffer(const void* data, u32 offset, u32 size) const
  {
    m_VertexBuffer->update(data, offset, size);

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
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");

    m_IndexBuffer = indexBuffer;

#if ENG_DEBUG
    unBind();
#endif
  }

  void OpenGLVertexArray::setIndexBuffer(const std::shared_ptr<StorageBuffer>& indexBufferStorage)
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");
    ENG_CORE_ASSERT(indexBufferStorage->type() == StorageBuffer::Type::IndexBuffer, "Submitted buffer is not an index buffer!");

    m_IndexBuffer = IndexBuffer(indexBufferStorage);

#if ENG_DEBUG
    unBind();
#endif
  }

  const BufferLayout& OpenGLVertexArray::getLayout() const
  {
    return m_VertexBufferLayout;
  }

  const std::optional<IndexBuffer>& OpenGLVertexArray::getIndexBuffer() const
  {
    return m_IndexBuffer;
  }
}