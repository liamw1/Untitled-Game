#include "ENpch.h"
#include "OpenGLStorageBuffer.h"
#include "Engine/Core/Casting.h"
#include "Engine/Debug/Assert.h"
#include "Engine/Threads/Threads.h"

#include <glad/glad.h>

namespace eng::mem
{
  static GLenum convertTypeToGLEnum(StorageBuffer::Type type)
  {
    switch (type)
    {
      case StorageBuffer::Type::Uniform:  return GL_UNIFORM_BUFFER;
      case StorageBuffer::Type::SSBO:     return GL_SHADER_STORAGE_BUFFER;
    }
    throw CoreException("Invalid storage buffer type!");
  }



  OpenGLStorageBuffer::OpenGLStorageBuffer(Type type, u32 binding, uSize size)
    : m_Type(type),
      m_Size(size),
      m_Binding(binding),
      m_BufferID(0)
  {
    ENG_CORE_ASSERT(thread::isMainThread(), "OpenGL calls must be made on the main thread!");

    glCreateBuffers(1, &m_BufferID);
    glNamedBufferStorage(m_BufferID, m_Size, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(convertTypeToGLEnum(m_Type), m_Binding, m_BufferID);
  }

  OpenGLStorageBuffer::~OpenGLStorageBuffer()
  {
    ENG_CORE_ASSERT(thread::isMainThread(), "OpenGL calls must be made on the main thread!");
    glDeleteBuffers(1, &m_BufferID);
  }

  OpenGLStorageBuffer::Type OpenGLStorageBuffer::type() const { return m_Type; }
  uSize OpenGLStorageBuffer::size() const { return m_Size; }

  void OpenGLStorageBuffer::write(const mem::RenderData& data)
  {
    ENG_CORE_ASSERT(data.size() <= m_Size, "Given data is exceeds capacity of the buffer!");

    GLsizeiptr writeSize = arithmeticCast<GLsizeiptr>(std::min(m_Size, data.size()));
    glNamedBufferSubData(m_BufferID, 0, writeSize, data.raw());
  }
}