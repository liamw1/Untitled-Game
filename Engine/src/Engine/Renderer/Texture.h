#pragma once
#include "Engine/Math/ArrayBox.h"
#include "Engine/Memory/Data.h"
#include "Engine/Utilities/Constraints.h"

/*
  Abstract representation of a texture.
  Platform-specific implementation is determined by derived class.
*/
namespace eng
{
  class Image : private SetInStone
  {
    math::ArrayBox<u8, i32> m_Data;

  public:
    Image(const std::filesystem::path& path);

    i32 width() const;
    i32 height() const;
    i32 channels() const;
    i32 pixelCount() const;

    const u8* data() const;

    math::Float4 averageColor() const;
  };



  class Texture
  {
  public:
    virtual ~Texture();

    virtual u32 getWidth() const = 0;
    virtual u32 getHeight() const = 0;
    virtual u32 getRendererID() const = 0;

    /*
      Method for setting texture data directly.
      \param data Buffer of texture data
      \param size Size of buffer in bytes
    */
    virtual void setData(const mem::Data& textureData) = 0;

    /*
      Binds texture to texture slot (0 by default).
      Number of slot varies between GPUs, usually around 16-32 total.
    */
    virtual void bind(u32 slot = 0) const = 0;

    virtual bool operator==(const Texture& other) const = 0;

    static std::unique_ptr<Texture> Create(u32 width, u32 height);
    static std::unique_ptr<Texture> Create(const std::filesystem::path& path);
  };



  class TextureArray
  {
  public:
    virtual ~TextureArray();

    virtual void bind(u32 slot = 0) const = 0;

    virtual void addTexture(const std::filesystem::path& path) = 0;
    virtual void addTexture(const Image& image) = 0;

    static std::unique_ptr<TextureArray> Create(u32 textureCount, u32 textureSize);
  };
}