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
      case mem::ShaderDataType::Bool:
        return GL_BOOL;
      case mem::ShaderDataType::Unsigned:
        return GL_UNSIGNED_INT;
      case mem::ShaderDataType::Int:
      case mem::ShaderDataType::Int2:
      case mem::ShaderDataType::Int3:
      case mem::ShaderDataType::Int4:
        return GL_INT;
      case mem::ShaderDataType::Float:
      case mem::ShaderDataType::Float2:
      case mem::ShaderDataType::Float3:
      case mem::ShaderDataType::Float4:
      case mem::ShaderDataType::Mat3:
      case mem::ShaderDataType::Mat4:
        return GL_FLOAT;
    }
    throw CoreException("Invalid ShaderDataType!");
  }

  OpenGLVertexArray::OpenGLVertexArray()
  {
    ENG_CORE_ASSERT(thread::isMainThread(), "OpenGL calls must be made on the main thread!");

    glCreateVertexArrays(1, &m_RendererID);
    m_VertexBuffer = mem::DynamicBuffer::Create(mem::DynamicBuffer::Type::Vertex);
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
      m_VertexBuffer->unbind();
    if (m_IndexBuffer)
      m_IndexBuffer->unbind();
  }

  void OpenGLVertexArray::setLayout(const mem::BufferLayout& layout)
  {
    ENG_CORE_ASSERT(thread::isMainThread(), "OpenGL calls must be made on the main thread!");

    m_VertexBufferLayout = layout;

    glBindVertexArray(m_RendererID);
    m_VertexBuffer->bind();

    for (u32 attributeIndex = 0; attributeIndex < m_VertexBufferLayout.elements().size(); ++attributeIndex)
    {
      const mem::BufferElement& element = m_VertexBufferLayout.elements()[attributeIndex];
      std::underlying_type_t<mem::ShaderDataType> dataTypeID = toUnderlying(element.type);

      if (dataTypeID >= toUnderlying(mem::ShaderDataType::FirstFloat) && dataTypeID <= toUnderlying(mem::ShaderDataType::LastFloat))
      {
        glEnableVertexAttribArray(attributeIndex);
        glVertexAttribPointer(attributeIndex,
                              element.getComponentCount(),
                              convertToOpenGLBaseType(element.type),
                              element.normalized ? GL_TRUE : GL_FALSE,
                              m_VertexBufferLayout.stride(),
                              std::bit_cast<const void*>(arithmeticUpcast<uSize>(element.offset)));
      }
      else if (dataTypeID >= toUnderlying(mem::ShaderDataType::FirstInt) && dataTypeID <= toUnderlying(mem::ShaderDataType::LastInt))
      {
        glEnableVertexAttribArray(attributeIndex);
        glVertexAttribIPointer(attributeIndex,
                               element.getComponentCount(),
                               convertToOpenGLBaseType(element.type),
                               m_VertexBufferLayout.stride(),
                               std::bit_cast<const void*>(arithmeticUpcast<uSize>(element.offset)));
      }
      else if (dataTypeID >= toUnderlying(mem::ShaderDataType::FirstMat) && dataTypeID <= toUnderlying(mem::ShaderDataType::LastMat))
      {
        i32 componentCount = element.getComponentCount();
        for (i32 i = 0; i < componentCount; ++i)
        {
          glEnableVertexAttribArray(attributeIndex);
          glVertexAttribPointer(attributeIndex,
                                componentCount,
                                convertToOpenGLBaseType(element.type),
                                element.normalized ? GL_TRUE : GL_FALSE,
                                m_VertexBufferLayout.stride(),
                                std::bit_cast<const void*>(arithmeticUpcast<uSize>(element.offset + sizeof(f32) * componentCount * i)));
          glVertexAttribDivisor(attributeIndex, 1);
        }
      }
      else
        throw CoreException("Invalid shader data type!");
    }

#if ENG_DEBUG
    unBind();
#endif
  }

  void OpenGLVertexArray::setVertexBuffer(const mem::RenderData& data)
  {
    m_VertexBuffer->set(data);
  }

  void OpenGLVertexArray::setVertexBuffer(const std::shared_ptr<mem::DynamicBuffer>& vertexBuffer)
  {
    m_VertexBuffer = vertexBuffer;
    setLayout(m_VertexBufferLayout);
  }

  void OpenGLVertexArray::modifyVertexBuffer(u32 offset, const mem::RenderData& data) const
  {
    m_VertexBuffer->modify(offset, data);
  }

  void OpenGLVertexArray::resizeVertexBuffer(u32 newSize)
  {
    m_VertexBuffer->resize(newSize);
    setLayout(m_VertexBufferLayout);
  }

  void OpenGLVertexArray::setIndexBuffer(const IndexBuffer& indexBuffer)
  {
    ENG_CORE_ASSERT(thread::isMainThread(), "OpenGL calls must be made on the main thread!");
    m_IndexBuffer = indexBuffer;
  }

  void OpenGLVertexArray::setIndexBuffer(const std::shared_ptr<mem::DynamicBuffer>& indexBuffer)
  {
    ENG_CORE_ASSERT(thread::isMainThread(), "OpenGL calls must be made on the main thread!");
    ENG_CORE_ASSERT(indexBuffer->type() == mem::DynamicBuffer::Type::Index, "Submitted buffer is not an index buffer!");
    m_IndexBuffer = IndexBuffer(indexBuffer);
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