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
    glCreateBuffers(1, &m_VertexBufferID);
  }

  OpenGLVertexArray::~OpenGLVertexArray()
  {
    glDeleteVertexArrays(1, &m_RendererID);
    glDeleteBuffers(1, &m_VertexBufferID);
    glDeleteBuffers(1, &m_IndexBufferID);
  }

  void OpenGLVertexArray::bind() const
  {
    glBindVertexArray(m_RendererID);
  }

  void OpenGLVertexArray::unBind() const
  {
    glBindVertexArray(0);
  }

  void OpenGLVertexArray::setLayout(const BufferLayout& layout)
  {
    glBindVertexArray(m_RendererID);
    glBindBuffer(GL_ARRAY_BUFFER, m_VertexBufferID);

    uint32_t vertexBufferIndex = 0;
    for (const auto& element : layout)
    {
      const int dataTypeID = static_cast<int>(element.type);

      if (dataTypeID >= static_cast<int>(ShaderDataType::FloatTypeBegin) && dataTypeID <= static_cast<int>(ShaderDataType::FloatTypeEnd))
      {
        glEnableVertexAttribArray(vertexBufferIndex);
        glVertexAttribPointer(vertexBufferIndex,
          element.getComponentCount(),
          convertToOpenGLBaseType(element.type),
          element.normalized ? GL_TRUE : GL_FALSE,
          layout.getStride(),
          (const void*)(const size_t)element.offset);
        vertexBufferIndex++;
      }
      else if (dataTypeID >= static_cast<int>(ShaderDataType::IntTypeBegin) && dataTypeID <= static_cast<int>(ShaderDataType::IntTypeEnd))
      {
        glEnableVertexAttribArray(vertexBufferIndex);
        glVertexAttribIPointer(vertexBufferIndex,
          element.getComponentCount(),
          convertToOpenGLBaseType(element.type),
          layout.getStride(),
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
            layout.getStride(),
            (const void*)(const size_t)(element.offset + sizeof(float) * count * i));
          glVertexAttribDivisor(vertexBufferIndex, 1);
          vertexBufferIndex++;
        }
      }
      else
        EN_CORE_ASSERT(false, ("Unknown shader data type!"));
    }
  }

  void OpenGLVertexArray::setVertexData(const void* data, uintptr_t size)
  {
    glBindBuffer(GL_ARRAY_BUFFER, m_VertexBufferID);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);
  }

  void OpenGLVertexArray::setIndexBuffer(const uint32_t* indices, uint32_t indexCount)
  {
    glBindVertexArray(m_RendererID);

    glCreateBuffers(1, &m_IndexBufferID);

    /*
      GL_ELEMENT_ARRAY_BUFFER is not valid without an actively bound VAO.
      Binding with GL_ARRAY_BUFFER allows the data to be loaded regardless of VAO state.
    */
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexBufferID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(uint32_t), indices, GL_DYNAMIC_DRAW);

    m_IndexCount = indexCount;
  }
}