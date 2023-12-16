#include "ENpch.h"
#include "Buffer.h"
#include "Engine/Renderer/RendererAPI.h"
#include "Platform/OpenGL/OpenGLBuffer.h"

namespace eng::mem
{
  DynamicBuffer::~DynamicBuffer() = default;
  std::unique_ptr<DynamicBuffer> DynamicBuffer::Create(Type type)
  {
    switch (RendererAPI::GetAPI())
    {
      case RendererAPI::API::OpenGL:          return std::make_unique<OpenGLDynamicBuffer>(type);
      case RendererAPI::API::OpenGL_Legacy:   return std::make_unique<OpenGLDynamicBuffer>(type);
    }
    throw CoreException("Invalid RendererAPI!");
  }



  StorageBuffer::~StorageBuffer() = default;
  std::unique_ptr<StorageBuffer> StorageBuffer::Create(Type type, u32 binding, uSize capacity)
  {
    switch (RendererAPI::GetAPI())
    {
      case RendererAPI::API::OpenGL:          return std::make_unique<OpenGLStorageBuffer>(type, binding, capacity);
      case RendererAPI::API::OpenGL_Legacy:   return std::make_unique<OpenGLStorageBuffer>(type, binding, capacity);
    }
    throw CoreException("Invalid RendererAPI!");
  }
}