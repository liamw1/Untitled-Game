#include "ENpch.h"
#include "StorageBuffer.h"
#include "Engine/Renderer/RendererAPI.h"
#include "Platform/OpenGL/OpenGLStorageBuffer.h"

namespace eng::mem
{
  StorageBuffer::~StorageBuffer() = default;
  std::unique_ptr<StorageBuffer> StorageBuffer::Create(Type type, u32 binding, uSize size)
  {
    switch (RendererAPI::GetAPI())
    {
      case RendererAPI::API::OpenGL:          return std::make_unique<OpenGLStorageBuffer>(type, binding, size);
      case RendererAPI::API::OpenGL_Legacy:   return std::make_unique<OpenGLStorageBuffer>(type, binding, size);
    }
    throw CoreException("Invalid RendererAPI!");
  }
}