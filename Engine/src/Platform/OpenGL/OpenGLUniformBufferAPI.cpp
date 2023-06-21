#include "ENpch.h"
#include "OpenGLUniformBufferAPI.h"
#include "Engine/Threading/Threads.h"
#include <glad/glad.h>

namespace Engine
{
  static GLint s_MaxUniformBlockSize = 0;

  OpenGLUniformBufferAPI::OpenGLUniformBufferAPI()
  {
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &s_MaxUniformBlockSize);
  }

  void OpenGLUniformBufferAPI::allocate(uint32_t binding, uint32_t size)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");
    EN_CORE_ASSERT(binding < c_MaxUniformBindings, "Binding exceeds maximum allowed uniform bindings!");

    if (m_RendererIDs[binding] != 0)
      EN_CORE_WARN("Uniform buffer has already been allocated at binding {0}!", binding);

    if (size > static_cast<uint32_t>(s_MaxUniformBlockSize))
    {
      EN_CORE_ERROR("Requested uniform buffer is larger than the maximum allowable uniform block size!");
      return;
    }

    uint32_t rendererID;
    glCreateBuffers(1, &rendererID);
    glNamedBufferData(rendererID, size, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, binding, rendererID);

    m_RendererIDs[binding] = rendererID;
    m_BufferSizes[binding] = size;
  }

  void OpenGLUniformBufferAPI::deallocate(uint32_t binding)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");
    EN_CORE_ASSERT(binding < c_MaxUniformBindings, "Binding exceeds maximum allowed uniform bindings!");

    glDeleteBuffers(1, &m_RendererIDs[binding]);
    m_RendererIDs[binding] = 0;
    m_BufferSizes[binding] = 0;
  }

  void OpenGLUniformBufferAPI::bind(uint32_t binding) const
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");
    EN_CORE_ASSERT(binding < c_MaxUniformBindings, "Binding exceeds maximum allowed uniform bindings!");

    glBindBuffer(GL_UNIFORM_BUFFER, m_RendererIDs[binding]);
  }

  void OpenGLUniformBufferAPI::unbind() const
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
  }

  int OpenGLUniformBufferAPI::maxSize() const
  {
    return s_MaxUniformBlockSize;
  }

  int OpenGLUniformBufferAPI::getSize(uint32_t binding) const
  {
    EN_CORE_ASSERT(binding < c_MaxUniformBindings, "Binding exceeds maximum allowed uniform bindings!");
    return m_BufferSizes[binding];
  }

  void OpenGLUniformBufferAPI::setData(uint32_t binding, const void* data, uint32_t size, uint32_t offset)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");
    EN_CORE_ASSERT(binding < c_MaxUniformBindings, "Binding exceeds maximum allowed uniform bindings!");
    EN_CORE_ASSERT(size + offset <= m_BufferSizes[binding], "Data exceeds uniform buffer size!");

    glNamedBufferSubData(m_RendererIDs[binding], offset, size, data);
  }
}