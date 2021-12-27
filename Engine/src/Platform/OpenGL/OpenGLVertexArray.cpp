#include "ENpch.h"
#include "OpenGLVertexArray.h"
#include <glad/glad.h>

namespace Engine
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
      default: EN_CORE_ASSERT(false, "Unknown ShaderDataType!"); return 0;
    }
  }

  OpenGLVertexArray::OpenGLVertexArray()
  {
    glCreateVertexArrays(1, &m_RendererID);
  }

  OpenGLVertexArray::~OpenGLVertexArray()
  {
    glDeleteVertexArrays(1, &m_RendererID);
  }

  void OpenGLVertexArray::bind() const
  {
    glBindVertexArray(m_RendererID);
  }

  void OpenGLVertexArray::unBind() const
  {
    glBindVertexArray(0);
  }

  void OpenGLVertexArray::addVertexBuffer(const Shared<VertexBuffer>& vertexBuffer)
  {
    EN_CORE_ASSERT(vertexBuffer->getLayout().getElements().size() > 0, "Vertex Buffer has no layout!");

    glBindVertexArray(m_RendererID);
    vertexBuffer->bind();

    const auto& layout = vertexBuffer->getLayout();
    for (const auto& element : layout)
    {
      const int dataTypeID = static_cast<int>(element.type);

      if (dataTypeID >= static_cast<int>(ShaderDataType::FloatTypeBegin) && dataTypeID <= static_cast<int>(ShaderDataType::FloatTypeEnd))
      {
        glEnableVertexAttribArray(m_VertexBufferIndex);
        glVertexAttribPointer(m_VertexBufferIndex,
          element.getComponentCount(),
          convertToOpenGLBaseType(element.type),
          element.normalized ? GL_TRUE : GL_FALSE,
          layout.getStride(),
          (const void*)(const size_t)element.offset);
        m_VertexBufferIndex++;
      }
      else if (dataTypeID >= static_cast<int>(ShaderDataType::IntTypeBegin) && dataTypeID <= static_cast<int>(ShaderDataType::IntTypeEnd))
      {
        glEnableVertexAttribArray(m_VertexBufferIndex);
        glVertexAttribIPointer(m_VertexBufferIndex,
          element.getComponentCount(),
          convertToOpenGLBaseType(element.type),
          layout.getStride(),
          (const void*)(const size_t)element.offset);
        m_VertexBufferIndex++;
      }
      else if (dataTypeID >= static_cast<int>(ShaderDataType::MatTypeBegin) && dataTypeID <= static_cast<int>(ShaderDataType::MatTypeEnd))
      {
        int count = element.getComponentCount();
        for (int i = 0; i < count; ++i)
        {
          glEnableVertexAttribArray(m_VertexBufferIndex);
          glVertexAttribPointer(m_VertexBufferIndex,
            count,
            convertToOpenGLBaseType(element.type),
            element.normalized ? GL_TRUE : GL_FALSE,
            layout.getStride(),
            (const void*)(const size_t)(element.offset + sizeof(float) * count * i));
          glVertexAttribDivisor(m_VertexBufferIndex, 1);
          m_VertexBufferIndex++;
        }
      }
      else
        EN_CORE_ASSERT(false, ("Unknown shader data type!"));
    }

    m_VertexBuffers.push_back(vertexBuffer);
  }

  void OpenGLVertexArray::setIndexBuffer(const Shared<IndexBuffer>& indexBuffer)
  {
    EN_CORE_ASSERT(indexBuffer->getCount() % 3 == 0, "Index buffer count is not a multiple of three!");

    glBindVertexArray(m_RendererID);
    indexBuffer->bind();
    m_IndexBuffer = indexBuffer;
  }
  const std::vector<Shared<VertexBuffer>>& OpenGLVertexArray::getBuffers() const
  {
    return m_VertexBuffers;
  }

  const Shared<IndexBuffer>& OpenGLVertexArray::getIndexBuffer() const
  {
    return m_IndexBuffer;
  }
}