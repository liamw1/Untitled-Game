#pragma once
#include "Engine/Renderer/Texture.h"
#include <glad/glad.h>

namespace Engine
{
  class OpenGLTextureArray : public TextureArray
  {
  public:
    OpenGLTextureArray(uint32_t textureCount, uint32_t textureSize);
    ~OpenGLTextureArray();

    void bind(uint32_t slot = 0) const override;

    void addTexture(const std::string& path) override;

  private:
    uint32_t m_MaxTextures, m_TextureSize;
    uint32_t m_TextureCount = 0;
    uint32_t m_RendererID = 0;
    GLenum m_InternalFormat = GL_RGBA8, m_DataFormat = GL_RGBA;
  };

  class OpenGLTexture2D : public Texture2D
  {
  public:
    OpenGLTexture2D(uint32_t width, uint32_t height);
    OpenGLTexture2D(const std::string& path);
    ~OpenGLTexture2D();

    uint32_t getWidth() const override { return m_Width; }
    uint32_t getHeight() const override { return m_Height; }
    uint32_t getRendererID() const override { return m_RendererID; }

    void setData(void* data, uint32_t size) override;

    void bind(uint32_t slot = 0) const override;

    bool operator==(const Texture& other) const override { return m_RendererID == ((OpenGLTexture2D&)other).m_RendererID; }

  private:
    uint32_t m_Width, m_Height;
    uint32_t m_RendererID = 0;
    GLenum m_InternalFormat, m_DataFormat;
  };
}