#include "ENpch.h"
#include "StorageBuffer.h"
#include "Engine/Renderer/RendererAPI.h"
#include "Platform/OpenGL/OpenGLStorageBuffer.h"

namespace eng
{
  StorageBuffer::~StorageBuffer() = default;

  std::unique_ptr<StorageBuffer> StorageBuffer::Create(Type type, std::optional<u32> binding)
  {
    switch (RendererAPI::GetAPI())
    {
      case RendererAPI::API::OpenGL:          return std::make_unique<OpenGLStorageBuffer>(type, binding);
      case RendererAPI::API::OpenGL_Legacy:   return std::make_unique<OpenGLStorageBuffer>(type, binding);
    }
    throw std::invalid_argument("Invalid RendererAPI!");
  }
}