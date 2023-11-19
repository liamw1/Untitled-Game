#pragma once
#include "Engine/Renderer/Texture.h"

namespace eng
{
  class OpenGLTexture : public Texture
  {
    u32 m_Width;
    u32 m_Height;
    u32 m_RendererID;
    u32 m_InternalFormat;
    u32 m_DataFormat;

  public:
    OpenGLTexture(u32 width, u32 height);
    OpenGLTexture(const std::filesystem::path& path);
    ~OpenGLTexture();

    u32 getWidth() const override;
    u32 getHeight() const override;
    u32 getRendererID() const override;

    void setData(const mem::Data& textureData) override;

    void bind(u32 slot = 0) const override;

    bool operator==(const Texture& other) const override;
  };



  class OpenGLTextureArray : public TextureArray
  {
    u32 m_MaxTextures;
    u32 m_TextureSize;
    u32 m_TextureCount;
    u32 m_RendererID;
    u32 m_InternalFormat;
    u32 m_DataFormat;

  public:
    OpenGLTextureArray(u32 textureCount, u32 textureSize);
    ~OpenGLTextureArray();

    void bind(u32 slot = 0) const override;

    void addTexture(const std::filesystem::path& path) override;
    void addTexture(const Image& image) override;
  };
}