#include "ENpch.h"
#include "StorageBuffer.h"
#include "RendererAPI.h"
#include "Platform/OpenGL/OpenGLStorageBuffer.h"

namespace Engine
{
  std::unique_ptr<StorageBuffer> StorageBuffer::Create(Type type, std::optional<uint32_t> binding)
  {
    switch (RendererAPI::GetAPI())
    {
      case RendererAPI::API::None:            EN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
      case RendererAPI::API::OpenGL:          return std::make_unique<OpenGLStorageBuffer>(type, binding);
      case RendererAPI::API::OpenGL_Legacy:   return std::make_unique<OpenGLStorageBuffer>(type, binding);
      default:                                EN_CORE_ASSERT(false, "Unknown RendererAPI!"); return nullptr;
    }
  }
}