#pragma once

/*
  Abstract representation of a texture.
  Platform-specific implementation is determined by derived class.
*/
namespace Engine
{
  class TextureArray
  {
  public:
    virtual void bind(uint32_t slot = 0) const = 0;

    virtual void addTexture(const std::string& path) = 0;

    static Shared<TextureArray> Create(uint32_t textureCount, uint32_t textureSize);
  };

  class Texture
  {
  public:
    virtual ~Texture() = default;

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
  };

  class Texture2D : public Texture
  {
  public:
    static Shared<Texture2D> Create(uint32_t width, uint32_t height);
    static Shared<Texture2D> Create(const std::string& path);
  };
}