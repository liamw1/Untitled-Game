#include "ENpch.h"
#include "OpenGLUniformBuffer.h"
#include "Engine/Threading/Threads.h"
#include <glad/glad.h>

namespace Engine
{
	OpenGLUniformBuffer::OpenGLUniformBuffer(uint32_t size, uint32_t binding)
	{
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::GetMainThreadID(), "OpenGL calls must be made in main thread!");

		glCreateBuffers(1, &m_RendererID);
		glNamedBufferData(m_RendererID, size, nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, binding, m_RendererID);
	}

	OpenGLUniformBuffer::~OpenGLUniformBuffer()
	{
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::GetMainThreadID(), "OpenGL calls must be made in main thread!");
		glDeleteBuffers(1, &m_RendererID);
	}

  void OpenGLUniformBuffer::bind() const
  {
    glBindBuffer(GL_UNIFORM_BUFFER, m_RendererID);
  }

  void OpenGLUniformBuffer::unBind() const
  {
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
  }


	void OpenGLUniformBuffer::setData(const void* data, uint32_t size, uint32_t offset)
	{
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::GetMainThreadID(), "OpenGL calls must be made in main thread!");
		glNamedBufferSubData(m_RendererID, offset, size, data);
	}
}