#include "ENpch.h"
#include "Texture.h"
#include "RendererAPI.h"
#include "TextureAPI.h"
#include "Platform/OpenGL/OpenGLTextureAPI.h"

namespace Engine
{
  static Unique<TextureAPI> s_TextureAPI = nullptr;

  void Engine::Texture::Initialize()
  {
    switch (RendererAPI::GetAPI())
    {
      case RendererAPI::API::None:          EN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return;
      case RendererAPI::API::OpenGL:        s_TextureAPI = CreateUnique<OpenGLTextureAPI>();  break;
      case RendererAPI::API::OpenGL_Legacy: s_TextureAPI = CreateUnique<OpenGLTextureAPI>();  break;
      default:                              EN_CORE_ASSERT(false, "Unknown RendererAPI!");                return;
    }
  }

  void Texture::Create2D(uint32_t binding, uint32_t width, uint32_t height)
  {
    s_TextureAPI->create2D(binding, width, height);
  }

  void Texture::Create2D(uint32_t binding, const std::string& path)
  {
    s_TextureAPI->create2D(binding, path);
  }

  void Texture::Create2DArray(uint32_t binding, uint32_t textureCount, uint32_t textureSize)
  {
    s_TextureAPI->create2DArray(binding, textureCount, textureSize);
  }

  void Texture::Remove(uint32_t binding)
  {
    s_TextureAPI->remove(binding);
  }

  void Texture::Bind(uint32_t binding)
  {
    s_TextureAPI->bind(binding);
  }

  void Texture::Add(uint32_t binding, const std::string& path)
  {
    s_TextureAPI->add(binding, path);
  }
}
