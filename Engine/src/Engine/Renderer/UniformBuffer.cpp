#include "ENpch.h"
#include "UniformBuffer.h"
#include "RendererAPI.h"
#include "UniformBufferAPI.h"
#include "Platform/OpenGL/OpenGLUniformBufferAPI.h"

namespace Engine
{
  static Unique<UniformBufferAPI> s_UniformBufferAPI = nullptr;

  void UniformBuffer::Initialize()
  {
    switch (RendererAPI::GetAPI())
    {
      case RendererAPI::API::None:          EN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return;
      case RendererAPI::API::OpenGL:        s_UniformBufferAPI = CreateUnique<OpenGLUniformBufferAPI>();  break;
      case RendererAPI::API::OpenGL_Legacy: s_UniformBufferAPI = CreateUnique<OpenGLUniformBufferAPI>();  break;
      default:                              EN_CORE_ASSERT(false, "Unknown RendererAPI!");                return;
    }
  }

  void UniformBuffer::Allocate(uint32_t binding, uint32_t size)
  {
    s_UniformBufferAPI->allocate(binding, size);
  }

  void UniformBuffer::Deallocate(uint32_t binding)
  {
    s_UniformBufferAPI->deallocate(binding);
  }

  void UniformBuffer::Bind(uint32_t binding)
  {
    s_UniformBufferAPI->bind(binding);
  }

  void UniformBuffer::Unbind()
  {
    s_UniformBufferAPI->unbind();
  }

  void UniformBuffer::SetData(uint32_t binding, const void* data)
  {
    SetData(binding, data, s_UniformBufferAPI->getSize(binding), 0);
  }

  void UniformBuffer::SetData(uint32_t binding, const void* data, uint32_t size, uint32_t offset)
  {
    s_UniformBufferAPI->setData(binding, data, size, offset);
  }
}