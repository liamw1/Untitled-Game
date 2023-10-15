#pragma once
#include "Engine/Math/ArrayBox.h"
#include "Engine/Utilities/Constraints.h"

/*
  Abstract representation of a texture.
  Platform-specific implementation is determined by derived class.
*/
namespace eng
{
  class Image : private NonCopyable, NonMovable
  {
  public:
    Image(const std::filesystem::path& path);

    int width() const;
    int height() const;
    int channels() const;
    int pixelCount() const;

    const uint8_t* data() const;

    math::Float4 averageColor() const;

  private:
    math::ArrayBox<uint8_t, int> m_Data;
  };



  class Texture
  {
  public:
    virtual ~Texture();

    virtual uint32_t getWidth() const = 0;
    virtual uint32_t getHeight() const = 0;
    virtual uint32_t getRendererID() const = 0;

    /*
      Method for setting texture data directly.
      \param data Buffer of texture data
      \param size Size of buffer in bytes
    */
    virtual void setData(void* data, uint32_t size) = 0;

    /*
      Binds texture to texture slot (0 by default).
      Number of slot varies between GPUs, usually around 16-32 total.
    */
    virtual void bind(uint32_t slot = 0) const = 0;

    virtual bool operator==(const Texture& other) const = 0;

    static std::unique_ptr<Texture> Create(uint32_t width, uint32_t height);
    static std::unique_ptr<Texture> Create(const std::filesystem::path& path);
  };



  class TextureArray
  {
  public:
    virtual ~TextureArray();

    virtual void bind(uint32_t slot = 0) const = 0;

    virtual void addTexture(const std::filesystem::path& path) = 0;
    virtual void addTexture(const Image& image) = 0;

    static std::unique_ptr<TextureArray> Create(uint32_t textureCount, uint32_t textureSize);
  };
}