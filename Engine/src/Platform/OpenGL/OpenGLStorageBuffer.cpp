#include "ENpch.h"
#include "OpenGLStorageBuffer.h"
#include "Engine/Threads/Threads.h"
#include <glad/glad.h>

namespace eng
{
  static GLenum convertTypeToGLEnum(StorageBuffer::Type type)
  {
    switch (type)
    {
      case StorageBuffer::Type::VertexBuffer:   return GL_ARRAY_BUFFER;
      case StorageBuffer::Type::IndexBuffer:    return GL_ELEMENT_ARRAY_BUFFER;
      case StorageBuffer::Type::SSBO:           return GL_SHADER_STORAGE_BUFFER;
      default:  ENG_CORE_ERROR("Invalid storage buffer type!"); return 0;
    }
  }

  OpenGLStorageBuffer::OpenGLStorageBuffer(Type type, std::optional<u32> binding)
    : m_Type(type),
      m_Size(0),
      m_Binding(binding),
      m_RendererID(0)
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");
    glCreateBuffers(1, &m_RendererID);

#if ENG_DEBUG
    unBind();
#endif
  }

  OpenGLStorageBuffer::~OpenGLStorageBuffer()
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");
    glDeleteBuffers(1, &m_RendererID);
  }

  void OpenGLStorageBuffer::bind() const
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");

    GLenum target = convertTypeToGLEnum(m_Type);

    glBindBuffer(convertTypeToGLEnum(m_Type), m_RendererID);
    if (m_Binding)
      glBindBufferBase(target, *m_Binding, m_RendererID);
  }

  void OpenGLStorageBuffer::unBind() const
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");
    glBindBuffer(convertTypeToGLEnum(m_Type), 0);
  }

  u32 OpenGLStorageBuffer::size() const
  {
    return m_Size;
  }

  StorageBuffer::Type OpenGLStorageBuffer::type() const
  {
    return m_Type;
  }

  void OpenGLStorageBuffer::set(const void* data, u32 size)
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");
    glNamedBufferData(m_RendererID, size, data, GL_DYNAMIC_DRAW);
    m_Size = size;

#if ENG_DEBUG
    unBind();
#endif
  }

  void OpenGLStorageBuffer::update(const void* data, u32 offset, u32 size)
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");
    ENG_CORE_ASSERT(offset + size <= m_Size, "Data is outside of buffer range!");
    glNamedBufferSubData(m_RendererID, offset, size, data);

#if ENG_DEBUG
    unBind();
#endif
  }

  void OpenGLStorageBuffer::resize(u32 newSize)
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");

    // Set up new vertex buffer
    u32 oldRendererID = m_RendererID;
    glCreateBuffers(1, &m_RendererID);
    glNamedBufferData(m_RendererID, newSize, nullptr, GL_DYNAMIC_DRAW);

    // Copy old buffer and free its memory
    glCopyNamedBufferSubData(oldRendererID, m_RendererID, 0, 0, std::min(m_Size, newSize));
    glDeleteBuffers(1, &oldRendererID);
    m_Size = newSize;

#if ENG_DEBUG
    unBind();
#endif
  }
}