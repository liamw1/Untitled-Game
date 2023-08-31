#include "ENpch.h"
#include "OpenGLUniform.h"
#include "Engine/Threads/Threads.h"
#include <glad/glad.h>

namespace Engine
{
  OpenGLUniform::OpenGLUniform(uint32_t binding, uint32_t size)
    : m_Binding(binding)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");
    EN_CORE_ASSERT(m_Binding < c_MaxUniformBindings, "Binding exceeds maximum allowed uniform bindings!");

    if (s_RendererIDs[m_Binding] != 0)
      EN_CORE_WARN("Uniform buffer has already been allocated at binding {0}!", m_Binding);

    if (size > c_MaxUniformBlockSize)
    {
      EN_CORE_ERROR("Requested uniform buffer is larger than the maximum allowable uniform block size!");
      return;
    }

    uint32_t rendererID;
    glCreateBuffers(1, &rendererID);
    glNamedBufferData(rendererID, size, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, m_Binding, rendererID);

    s_RendererIDs[m_Binding] = rendererID;
    s_BufferSizes[m_Binding] = size;
  }

  OpenGLUniform::~OpenGLUniform()
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");
    EN_CORE_ASSERT(m_Binding < c_MaxUniformBindings, "Binding exceeds maximum allowed uniform bindings!");

    glDeleteBuffers(1, &s_RendererIDs[m_Binding]);
    s_RendererIDs[m_Binding] = 0;
    s_BufferSizes[m_Binding] = 0;
  }

  void OpenGLUniform::bind() const
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");
    EN_CORE_ASSERT(m_Binding < c_MaxUniformBindings, "Binding exceeds maximum allowed uniform bindings!");

    glBindBuffer(GL_UNIFORM_BUFFER, s_RendererIDs[m_Binding]);
  }

  void OpenGLUniform::unBind() const
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
  }

  void OpenGLUniform::set(const void* data, uint32_t size)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");
    EN_CORE_ASSERT(m_Binding < c_MaxUniformBindings, "Binding exceeds maximum allowed uniform bindings!");
    EN_CORE_ASSERT(size <= s_BufferSizes[m_Binding], "Data exceeds uniform buffer size!");

    glNamedBufferSubData(s_RendererIDs[m_Binding], 0, size, data);
  }
}
