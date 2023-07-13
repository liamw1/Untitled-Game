#include "ENpch.h"
#include "StorageBuffer.h"
#include "RendererAPI.h"
#include "Platform/OpenGL/OpenGLStorageBuffer.h"

namespace Engine
{
  StorageBuffer::~StorageBuffer() = default;

  void StorageBuffer::update(const void* data, uint64_t offset, uint64_t size)
  {
    EN_ASSERT(offset < std::numeric_limits<uint32_t>::max(), "Requested buffer offset is greater than 32-bit unsigned integer limit!");
    EN_ASSERT(size < std::numeric_limits<uint32_t>::max(), "Requested buffer size is greater than 32-bit unsigned integer limit!");
    update(data, static_cast<uint32_t>(offset), static_cast<uint32_t>(size));
  }

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