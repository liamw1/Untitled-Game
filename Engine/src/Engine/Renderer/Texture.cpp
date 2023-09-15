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
      case RendererAPI::API::OpenGL:        return std::make_unique<OpenGLTexture2D>(width, height);
      case RendererAPI::API::OpenGL_Legacy: return std::make_unique<OpenGLTexture2D>(width, height);
      default:                              throw  std::invalid_argument("Unknown RendererAPI!");
    }
  }

  std::unique_ptr<Texture2D> Texture2D::Create(const std::string& path)
  {
    switch (RendererAPI::GetAPI())
    {
      case RendererAPI::API::OpenGL:        return std::make_unique<OpenGLTexture2D>(path);
      case RendererAPI::API::OpenGL_Legacy: return std::make_unique<OpenGLTexture2D>(path);
      default:                              throw  std::invalid_argument("Unknown RendererAPI!");
    }
  }



  TextureArray::~TextureArray() = default;

  std::unique_ptr<TextureArray> TextureArray::Create(uint32_t textureCount, uint32_t textureSize)
  {
    switch (RendererAPI::GetAPI())
    {
      case RendererAPI::API::OpenGL:        return std::make_unique<OpenGLTextureArray>(textureCount, textureSize);
      case RendererAPI::API::OpenGL_Legacy: return std::make_unique<OpenGLTextureArray>(textureCount, textureSize);
      default:                              throw  std::invalid_argument("Unknown RendererAPI!");
    }
  }
}