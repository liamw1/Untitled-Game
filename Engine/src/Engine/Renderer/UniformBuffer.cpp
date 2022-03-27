#include "ENpch.h"
#include "UniformBuffer.h"
#include "Renderer.h"
#include <Platform/OpenGL/OpenGLUniformBuffer.h>

namespace Engine
{
  Unique<UniformBuffer> UniformBuffer::Create(uint32_t size, uint32_t binding)
  {
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    EN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateUnique<OpenGLUniformBuffer>(size, binding);
			default:                        EN_CORE_ASSERT(false, "Unknown RendererAPI!");return nullptr;
		}
  }
}