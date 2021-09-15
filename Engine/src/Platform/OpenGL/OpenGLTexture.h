#pragma once
#include "Engine/Renderer/Texture.h"

namespace Engine
{
  class OpenGLTexture2D : public Texture2D
  {
  public:
    OpenGLTexture2D(const std::string& path);
    ~OpenGLTexture2D();

    inline uint32_t getWidth() const override { return m_Width; }
    inline uint32_t getHeight() const override { return m_Height; }

    void bind(uint32_t slot = 0) const override;

  private:
    uint32_t m_Width, m_Height = 0;
    uint32_t m_RendererID = 0;
  };
}