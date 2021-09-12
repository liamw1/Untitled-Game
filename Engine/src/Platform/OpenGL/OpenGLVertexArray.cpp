#include "ENpch.h"
#include "OpenGLVertexArray.h"
#include <glad/glad.h>

namespace Engine
{
  static GLenum convertToOpenGLBaseType(ShaderDataType type)
  {
    switch (type)
    {
      case ShaderDataType::Bool:        return GL_BOOL;   break;
      case ShaderDataType::Int:         return GL_INT;    break;
      case ShaderDataType::Int2:        return GL_INT;    break;
      case ShaderDataType::Int3:        return GL_INT;    break;
      case ShaderDataType::Int4:        return GL_INT;    break;
      case ShaderDataType::Float:       return GL_FLOAT;  break;
      case ShaderDataType::Float2:      return GL_FLOAT;  break;
      case ShaderDataType::Float3:      return GL_FLOAT;  break;
      case ShaderDataType::Float4:      return GL_FLOAT;  break;
      case ShaderDataType::Mat3:        return GL_FLOAT;  break;
      case ShaderDataType::Mat4:        return GL_FLOAT;  break;
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

  void OpenGLVertexArray::addVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer)
  {
    EN_CORE_ASSERT(vertexBuffer->getLayout().getElements().size() > 0, "Vertex Buffer has no layout!");

    glBindVertexArray(m_RendererID);
    vertexBuffer->bind();

    uint32_t index = 0;
    const auto& layout = vertexBuffer->getLayout();
    for (const auto& element : layout)
    {
      glEnableVertexAttribArray(index);
      glVertexAttribPointer(index,
        element.getComponentCount(),
        convertToOpenGLBaseType(element.type),
        element.normalized ? GL_TRUE : GL_FALSE,
        layout.getStride(),
        (const void*)element.offset);
      index++;
    }

    m_VertexBuffers.emplace_back(vertexBuffer);
  }

  void OpenGLVertexArray::setIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer)
  {
    glBindVertexArray(m_RendererID);
    indexBuffer->bind();
    m_IndexBuffer = indexBuffer;
  }
  const std::vector<std::shared_ptr<VertexBuffer>>& OpenGLVertexArray::getBuffers() const
  {
    return m_VertexBuffers;
  }

  const std::shared_ptr<IndexBuffer>& OpenGLVertexArray::getIndexBuffer() const
  {
    return m_IndexBuffer;
  }
}