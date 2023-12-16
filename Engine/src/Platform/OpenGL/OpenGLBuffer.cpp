#include "ENpch.h"
#include "OpenGLBuffer.h"
#include "Engine/Core/Casting.h"
#include "Engine/Debug/Assert.h"
#include "Engine/Threads/Threads.h"

#include <glad/glad.h>

namespace eng::mem
{
  static GLenum convertTypeToGLEnum(DynamicBuffer::Type type)
  {
    switch (type)
    {
      case DynamicBuffer::Type::Vertex:  return GL_ARRAY_BUFFER;
      case DynamicBuffer::Type::Index:   return GL_ELEMENT_ARRAY_BUFFER;
    }
    throw CoreException("Invalid draw buffer type!");
  }

  static GLenum convertTypeToGLEnum(StorageBuffer::Type type)
  {
    switch (type)
    {
      case StorageBuffer::Type::Uniform:  return GL_UNIFORM_BUFFER;
      case StorageBuffer::Type::SSBO:     return GL_SHADER_STORAGE_BUFFER;
    }
    throw CoreException("Invalid storage buffer type!");
  }



  OpenGLDynamicBuffer::OpenGLDynamicBuffer(Type type)
    : m_Type(type),
      m_Size(0),
      m_RendererID(0)
  {
    ENG_CORE_ASSERT(thread::isMainThread(), "OpenGL calls must be made on the main thread!");
    glCreateBuffers(1, &m_RendererID);
  }

  OpenGLDynamicBuffer::~OpenGLDynamicBuffer()
  {
    ENG_CORE_ASSERT(thread::isMainThread(), "OpenGL calls must be made on the main thread!");
    glDeleteBuffers(1, &m_RendererID);
  }

  void OpenGLDynamicBuffer::bind() const
  {
    ENG_CORE_ASSERT(thread::isMainThread(), "OpenGL calls must be made on the main thread!");
    glBindBuffer(convertTypeToGLEnum(m_Type), m_RendererID);
  }

  void OpenGLDynamicBuffer::unbind() const
  {
    ENG_CORE_ASSERT(thread::isMainThread(), "OpenGL calls must be made on the main thread!");
    glBindBuffer(convertTypeToGLEnum(m_Type), 0);
  }

  DynamicBuffer::Type OpenGLDynamicBuffer::type() const { return m_Type; }
  uSize OpenGLDynamicBuffer::size() const { return m_Size; }

  void OpenGLDynamicBuffer::set(const mem::RenderData& data)
  {
    ENG_CORE_ASSERT(thread::isMainThread(), "OpenGL calls must be made on the main thread!");
    glNamedBufferData(m_RendererID, data.size(), data.raw(), GL_DYNAMIC_DRAW);
    m_Size = arithmeticCast<u32>(data.size());
  }

  void OpenGLDynamicBuffer::modify(u32 offset, const mem::RenderData & data)
  {
    ENG_CORE_ASSERT(thread::isMainThread(), "OpenGL calls must be made on the main thread!");
    if (offset + data.size() > m_Size)
      throw CoreException("Data is outside of buffer range!");
    glNamedBufferSubData(m_RendererID, offset, data.size(), data.raw());
  }

  void OpenGLDynamicBuffer::resize(uSize newSize)
  {
    ENG_CORE_ASSERT(thread::isMainThread(), "OpenGL calls must be made on the main thread!");

    // Set up new vertex buffer
    u32 oldRendererID = m_RendererID;
    glCreateBuffers(1, &m_RendererID);
    glNamedBufferData(m_RendererID, newSize, nullptr, GL_DYNAMIC_DRAW);

    // Copy old buffer and free its memory
    if (oldRendererID)
    {
      glCopyNamedBufferSubData(oldRendererID, m_RendererID, 0, 0, std::min(m_Size, newSize));
      glDeleteBuffers(1, &oldRendererID);
    }
    m_Size = newSize;
  }



  OpenGLStorageBuffer::OpenGLStorageBuffer(Type type, u32 binding, uSize capacity)
    : m_Type(type),
      m_Size(0),
      m_Capacity(capacity),
      m_Binding(binding),
      m_RendererID(0)
  {
    ENG_CORE_ASSERT(thread::isMainThread(), "OpenGL calls must be made on the main thread!");

    glCreateBuffers(1, &m_RendererID);
    glNamedBufferStorage(m_RendererID, m_Capacity, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(convertTypeToGLEnum(m_Type), m_Binding, m_RendererID);
  }

  OpenGLStorageBuffer::~OpenGLStorageBuffer()
  {
    ENG_CORE_ASSERT(thread::isMainThread(), "OpenGL calls must be made on the main thread!");
    glDeleteBuffers(1, &m_RendererID);
  }

  OpenGLStorageBuffer::Type OpenGLStorageBuffer::type() const { return m_Type; }
  uSize OpenGLStorageBuffer::size() const { return m_Size; }
  uSize OpenGLStorageBuffer::capacity() const { return m_Capacity; }

  void OpenGLStorageBuffer::set(const mem::RenderData& data)
  {
    ENG_CORE_ASSERT(data.size() <= m_Capacity, "Given data is exceeds capacity of the buffer!");

    GLsizeiptr writeSize = arithmeticCast<GLsizeiptr>(std::min(m_Capacity, data.size()));
    glNamedBufferSubData(m_RendererID, 0, writeSize, data.raw());
    m_Size = writeSize;
  }
}