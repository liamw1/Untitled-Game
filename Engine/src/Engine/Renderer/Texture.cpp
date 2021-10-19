#include "ENpch.h"
#include "Texture.h"
#include "Renderer.h"
#include "Platform/OpenGL/OpenGLTexture.h"

namespace Engine
{
  Shared<TextureArray> TextureArray::Create(uint32_t textureCount, uint32_t textureSize)
  {
    switch (Renderer::GetAPI())
    {
      case RendererAPI::API::None:    EN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
      case RendererAPI::API::OpenGL:  return CreateShared<OpenGLTextureArray>(textureCount, textureSize);
      default:                        EN_CORE_ASSERT(false, "Unknown RendererAPI!"); return nullptr;
    }
  }

  Shared<Texture2D> Texture2D::Create(uint32_t width, uint32_t height)
  {
    switch (Renderer::GetAPI())
    {
      case RendererAPI::API::None:    EN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
      case RendererAPI::API::OpenGL:  return CreateShared<OpenGLTexture2D>(width, height);
      default:                        EN_CORE_ASSERT(false, "Unknown RendererAPI!"); return nullptr;
    }
  }

  Shared<Texture2D> Texture2D::Create(const std::string& path)
  {
    switch (Renderer::GetAPI())
    {
      case RendererAPI::API::None:    EN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
      case RendererAPI::API::OpenGL:  return CreateShared<OpenGLTexture2D>(path);
      default:                        EN_CORE_ASSERT(false, "Unknown RendererAPI!"); return nullptr;
    }
  }
}