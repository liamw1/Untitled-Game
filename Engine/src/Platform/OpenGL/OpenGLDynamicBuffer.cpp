#include "ENpch.h"
#include "OpenGLDynamicBuffer.h"
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



  OpenGLDynamicBuffer::OpenGLDynamicBuffer(Type type)
    : m_Type(type),
      m_Size(0),
      m_BufferID(0)
  {
    ENG_CORE_ASSERT(thread::isMainThread(), "OpenGL calls must be made on the main thread!");
    glCreateBuffers(1, &m_BufferID);
  }

  OpenGLDynamicBuffer::~OpenGLDynamicBuffer()
  {
    ENG_CORE_ASSERT(thread::isMainThread(), "OpenGL calls must be made on the main thread!");
    glDeleteBuffers(1, &m_BufferID);
  }

  void OpenGLDynamicBuffer::bind() const
  {
    ENG_CORE_ASSERT(thread::isMainThread(), "OpenGL calls must be made on the main thread!");
    glBindBuffer(convertTypeToGLEnum(m_Type), m_BufferID);
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
    glNamedBufferData(m_BufferID, data.size(), data.raw(), GL_DYNAMIC_DRAW);
    m_Size = arithmeticCast<u32>(data.size());
  }

  void OpenGLDynamicBuffer::modify(u32 offset, const mem::RenderData & data)
  {
    ENG_CORE_ASSERT(thread::isMainThread(), "OpenGL calls must be made on the main thread!");
    if (offset + data.size() > m_Size)
      throw CoreException("Data is outside of buffer range!");
    glNamedBufferSubData(m_BufferID, offset, data.size(), data.raw());
  }

  void OpenGLDynamicBuffer::resize(uSize newSize)
  {
    ENG_CORE_ASSERT(thread::isMainThread(), "OpenGL calls must be made on the main thread!");

    // Set up new vertex buffer
    u32 oldRendererID = m_BufferID;
    glCreateBuffers(1, &m_BufferID);
    glNamedBufferData(m_BufferID, newSize, nullptr, GL_DYNAMIC_DRAW);

    // Copy old buffer and free its memory
    if (oldRendererID)
    {
      glCopyNamedBufferSubData(oldRendererID, m_BufferID, 0, 0, std::min(m_Size, newSize));
      glDeleteBuffers(1, &oldRendererID);
    }
    m_Size = newSize;
  }
}