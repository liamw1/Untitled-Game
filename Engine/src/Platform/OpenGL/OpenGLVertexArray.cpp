#include "ENpch.h"
#include "OpenGLVertexArray.h"
#include "Engine/Threading/Threads.h"
#include <glad/glad.h>

namespace Engine
{
  OpenGLIndexBuffer::OpenGLIndexBuffer(const uint32_t* indices, uint32_t count)
    : m_Count(count)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");

    glCreateBuffers(1, &m_RendererID);

    /*
      GL_ELEMENT_ARRAY_BUFFER is not valid without an actively bound VAO.
      Binding with GL_ARRAY_BUFFER allows the data to be loaded regardless of VAO state.
    */
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * count, indices, GL_DYNAMIC_DRAW);

#if EN_DEBUG
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
#endif
  }

  OpenGLIndexBuffer::~OpenGLIndexBuffer()
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");
    glDeleteBuffers(1, &m_RendererID);
  }

  void OpenGLIndexBuffer::bind() const
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
  }

  void OpenGLIndexBuffer::unBind() const
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }



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
      default: EN_CORE_ASSERT(false, "Unknown ShaderDataType!"); return 0;
    }
  }

  OpenGLVertexArray::OpenGLVertexArray()
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");

    glCreateVertexArrays(1, &m_RendererID);
    glCreateBuffers(1, &m_VertexBufferID);
  }

  OpenGLVertexArray::~OpenGLVertexArray()
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");

    glDeleteVertexArrays(1, &m_RendererID);
    glDeleteBuffers(1, &m_VertexBufferID);
  }

  void OpenGLVertexArray::bind() const
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");

    if (m_VertexData)
      setVertexBuffer(m_VertexData.get(), m_VertexDataSize);
    glBindVertexArray(m_RendererID);

    if (m_IndexBuffer)
      m_IndexBuffer->bind();
  }

  void OpenGLVertexArray::unBind() const
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");

    glBindVertexArray(0);

    if (m_IndexBuffer)
      m_IndexBuffer->bind();
  }

  void OpenGLVertexArray::clear() const
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "Clear should be called on the main thread!");

    // TODO: If clear() is called off the main thread, the stored vertex data will be cleared but the buffer
    // on the GPU will not until the next call to bind() or setVertexBuffer(). Need to find a fix for this.

    setVertexBuffer(nullptr, 0);
  }

  void OpenGLVertexArray::setLayout(const BufferLayout& layout)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");

    glBindVertexArray(m_RendererID);
    glBindBuffer(GL_ARRAY_BUFFER, m_VertexBufferID);

    uint32_t vertexBufferIndex = 0;
    for (const BufferElement& element : layout)
    {
      const int dataTypeID = static_cast<int>(element.type);

      if (dataTypeID >= static_cast<int>(ShaderDataType::FloatTypeBegin) && dataTypeID <= static_cast<int>(ShaderDataType::FloatTypeEnd))
      {
        glEnableVertexAttribArray(vertexBufferIndex);
        glVertexAttribPointer(vertexBufferIndex,
          element.getComponentCount(),
          convertToOpenGLBaseType(element.type),
          element.normalized ? GL_TRUE : GL_FALSE,
          layout.stride(),
          (const void*)(const size_t)element.offset);
        vertexBufferIndex++;
      }
      else if (dataTypeID >= static_cast<int>(ShaderDataType::IntTypeBegin) && dataTypeID <= static_cast<int>(ShaderDataType::IntTypeEnd))
      {
        glEnableVertexAttribArray(vertexBufferIndex);
        glVertexAttribIPointer(vertexBufferIndex,
          element.getComponentCount(),
          convertToOpenGLBaseType(element.type),
          layout.stride(),
          (const void*)(const size_t)element.offset);
        vertexBufferIndex++;
      }
      else if (dataTypeID >= static_cast<int>(ShaderDataType::MatTypeBegin) && dataTypeID <= static_cast<int>(ShaderDataType::MatTypeEnd))
      {
        int count = element.getComponentCount();
        for (int i = 0; i < count; ++i)
        {
          glEnableVertexAttribArray(vertexBufferIndex);
          glVertexAttribPointer(vertexBufferIndex,
            count,
            convertToOpenGLBaseType(element.type),
            element.normalized ? GL_TRUE : GL_FALSE,
            layout.stride(),
            (const void*)(const size_t)(element.offset + sizeof(float) * count * i));
          glVertexAttribDivisor(vertexBufferIndex, 1);
          vertexBufferIndex++;
        }
      }
      else
        EN_CORE_ASSERT(false, ("Unknown shader data type!"));
    }

#if EN_DEBUG
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
#endif
  }

  void OpenGLVertexArray::setVertexBuffer(const void* data, uint32_t size) const
  {
    if (std::this_thread::get_id() != Threads::MainThreadID())
    {
      m_VertexData = std::make_unique_for_overwrite<char[]>(size);
      m_VertexDataSize = size;
      std::memcpy(m_VertexData.get(), data, size);
    }
    else
    {
      glBindVertexArray(m_RendererID);
      glBindBuffer(GL_ARRAY_BUFFER, m_VertexBufferID);
      glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);

      clearStoredVertexData();

#if EN_DEBUG
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glBindVertexArray(0);
#endif
    }
  }

  void OpenGLVertexArray::modifyVertexBuffer(const void* data, uint32_t offset, uint32_t size) const
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");

    glBindVertexArray(m_RendererID);
    glBindBuffer(GL_ARRAY_BUFFER, m_VertexBufferID);
    glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);

#if EN_DEBUG
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
#endif
  }

  void OpenGLVertexArray::setIndexBuffer(const std::shared_ptr<const IndexBuffer>& indexBuffer)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");

    glBindVertexArray(m_RendererID);
    indexBuffer->bind();
    m_IndexBuffer = indexBuffer;

#if EN_DEBUG
    m_IndexBuffer->unBind();
    glBindVertexArray(0);
#endif
  }

  void OpenGLVertexArray::clearStoredVertexData() const
  {
    m_VertexData.reset();
    m_VertexDataSize = 0;
  }
}