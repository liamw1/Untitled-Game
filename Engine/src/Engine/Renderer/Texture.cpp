#include "ENpch.h"
#include "Texture.h"
#include "RendererAPI.h"
#include "Platform/OpenGL/OpenGLTexture.h"

namespace Engine
{
  Texture::~Texture() = default;

  std::unique_ptr<Texture2D> Texture2D::Create(uint32_t width, uint32_t height)
  {
    switch (RendererAPI::GetAPI())
    {
      case RendererAPI::API::None:          EN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
      case RendererAPI::API::OpenGL:        return std::make_unique<OpenGLTexture2D>(width, height);
      case RendererAPI::API::OpenGL_Legacy: return std::make_unique<OpenGLTexture2D>(width, height);
      default:                              EN_CORE_ASSERT(false, "Unknown RendererAPI!"); return nullptr;
    }
  }

  std::unique_ptr<Texture2D> Texture2D::Create(const std::string& path)
  {
    switch (RendererAPI::GetAPI())
    {
      case RendererAPI::API::None:          EN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
      case RendererAPI::API::OpenGL:        return std::make_unique<OpenGLTexture2D>(path);
      case RendererAPI::API::OpenGL_Legacy: return std::make_unique<OpenGLTexture2D>(path);
      default:                              EN_CORE_ASSERT(false, "Unknown RendererAPI!"); return nullptr;
    }
  }



  TextureArray::~TextureArray() = default;

  std::unique_ptr<TextureArray> TextureArray::Create(uint32_t textureCount, uint32_t textureSize)
  {
    switch (RendererAPI::GetAPI())
    {
      case RendererAPI::API::None:          EN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
      case RendererAPI::API::OpenGL:        return std::make_unique<OpenGLTextureArray>(textureCount, textureSize);
      case RendererAPI::API::OpenGL_Legacy: return std::make_unique<OpenGLTextureArray>(textureCount, textureSize);
      default:                              EN_CORE_ASSERT(false, "Unknown RendererAPI!"); return nullptr;
    }
  }
}