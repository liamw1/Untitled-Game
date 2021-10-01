#include "ENpch.h"
#include "OpenGLBuffer.h"
#include <glad/glad.h>

namespace Engine
{
  OpenGLVertexBuffer::OpenGLVertexBuffer(uint32_t size)
  {
    EN_PROFILE_FUNCTION();

    glCreateBuffers(1, &m_RendererID);
    glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
    glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
  }

  OpenGLVertexBuffer::OpenGLVertexBuffer(float* vertices, uint32_t size)
  {
    EN_PROFILE_FUNCTION();

    glCreateBuffers(1, &m_RendererID);
    glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
    glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
  }

  OpenGLVertexBuffer::~OpenGLVertexBuffer()
  {
    EN_PROFILE_FUNCTION();

    glDeleteBuffers(1, &m_RendererID);
  }

  void OpenGLVertexBuffer::bind() const
  {
    glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
  }

  void OpenGLVertexBuffer::unBind() const
  {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }

  void OpenGLVertexBuffer::setData(const void* data, uintptr_t size)
  {
    EN_PROFILE_FUNCTION();

    glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
    glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
  }



  OpenGLIndexBuffer::OpenGLIndexBuffer(uint32_t* indices, uint32_t count)
    : m_Count(count)
  {
    EN_PROFILE_FUNCTION();

    glCreateBuffers(1, &m_RendererID);

    /*
      GL_ELEMENT_ARRAY_BUFFER is not valid without an actively bound VAO.
      Binding with GL_ARRAY_BUFFER allows the data to be loaded regardless of VAO state.
    */
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
  }

  OpenGLIndexBuffer::~OpenGLIndexBuffer()
  {
    EN_PROFILE_FUNCTION();

    glDeleteBuffers(1, &m_RendererID);
  }

  void OpenGLIndexBuffer::bind() const
  {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
  }

  void OpenGLIndexBuffer::unBind() const
  {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }
}