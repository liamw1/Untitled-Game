#pragma once
#include "Engine/Renderer/Texture.h"

namespace Engine
{
  class OpenGLTexture2D : public Texture2D
  {
  public:
    OpenGLTexture2D(uint32_t width, uint32_t height);
    OpenGLTexture2D(const std::string& path);
    ~OpenGLTexture2D();

    uint32_t getWidth() const override;
    uint32_t getHeight() const override;
    uint32_t getRendererID() const override;

    void setData(void* data, uint32_t size) override;

    void bind(uint32_t slot = 0) const override;

    bool operator==(const Texture& other) const override;

  private:
    uint32_t m_Width;
    uint32_t m_Height;
    uint32_t m_RendererID;
    uint32_t m_InternalFormat;
    uint32_t m_DataFormat;
  };



  class OpenGLTextureArray : public TextureArray
  {
  public:
    OpenGLTextureArray(uint32_t textureCount, uint32_t textureSize);
    ~OpenGLTextureArray();

    void bind(uint32_t slot = 0) const override;

    void addTexture(const std::string& path) override;

  private:
    uint32_t m_MaxTextures;
    uint32_t m_TextureSize;
    uint32_t m_TextureCount;
    uint32_t m_RendererID;
    uint32_t m_InternalFormat;
    uint32_t m_DataFormat;
  };
}