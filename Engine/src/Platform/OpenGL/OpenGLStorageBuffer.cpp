#include "ENpch.h"
#include "OpenGLStorageBuffer.h"
#include "Engine/Threading/Threads.h"
#include <glad/glad.h>

namespace Engine
{
  static GLenum convertTypeToGLEnum(StorageBuffer::Type type)
  {
    return type == StorageBuffer::Type::VertexBuffer ? GL_ARRAY_BUFFER : GL_SHADER_STORAGE_BUFFER;
  }

  OpenGLStorageBuffer::OpenGLStorageBuffer(Type type, std::optional<uint32_t> binding)
    : m_Type(type),
      m_Binding(binding),
      m_RendererID(0)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");
    glCreateBuffers(1, &m_RendererID);

#if EN_DEBUG
    unBind();
#endif
  }

  OpenGLStorageBuffer::~OpenGLStorageBuffer()
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");
    glDeleteBuffers(1, &m_RendererID);
  }

  void OpenGLStorageBuffer::bind() const
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");

    GLenum target = convertTypeToGLEnum(m_Type);

    glBindBuffer(convertTypeToGLEnum(m_Type), m_RendererID);
    if (m_Binding)
      glBindBufferBase(target, *m_Binding, m_RendererID);
  }

  void OpenGLStorageBuffer::unBind() const
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");
    glBindBuffer(convertTypeToGLEnum(m_Type), 0);
  }

  void OpenGLStorageBuffer::set(const void* data, uint32_t size)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");
    glNamedBufferData(m_RendererID, size, data, GL_DYNAMIC_DRAW);

#if EN_DEBUG
    unBind();
#endif
  }

  void OpenGLStorageBuffer::update(const void* data, uint32_t offset, uint32_t size)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");
    glNamedBufferSubData(m_RendererID, offset, size, data);

#if EN_DEBUG
    unBind();
#endif
  }

  void OpenGLStorageBuffer::resize(uint32_t newSize)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");

    // Prepare old vertex buffer to be read from
    GLint oldSize;
    uint32_t oldRendererID = m_RendererID;
    glGetNamedBufferParameteriv(oldRendererID, GL_BUFFER_SIZE, &oldSize);

    // Set up new vertex buffer
    glCreateBuffers(1, &m_RendererID);
    glNamedBufferData(m_RendererID, newSize, nullptr, GL_DYNAMIC_DRAW);

    // Copy old buffer and free its memory
    glCopyNamedBufferSubData(oldRendererID, m_RendererID, 0, 0, std::min(oldSize, static_cast<GLint>(newSize)));
    glDeleteBuffers(1, &oldRendererID);

#if EN_DEBUG
    unBind();
#endif
  }
}