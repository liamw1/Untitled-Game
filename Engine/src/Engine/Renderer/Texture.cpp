#include "ENpch.h"
#include "Texture.h"
#include "RendererAPI.h"
#include "Platform/OpenGL/OpenGLTexture.h"
#include "Engine/Debug/Instrumentor.h"

#include <stb_image.h>

namespace eng
{
  Image::Image(const std::filesystem::path& path)
    : m_Data({}, AllocationPolicy::Deferred)
  {
    ENG_PROFILE_FUNCTION();

    int width, height, channels;

    // Load image data
    stbi_set_flip_vertically_on_load(true);
    stbi_uc* imageData = stbi_load(path.string().c_str(), &width, &height, &channels, 0);
    ENG_CORE_ASSERT(imageData, "Failed to load image at {0}.", path);

    // Copy data into ArrayBox
    math::IBox3<int> imageBounds(0, 0, 0, height - 1, width - 1, channels - 1);
    m_Data = math::ArrayBox<uint8_t, int>(imageBounds, AllocationPolicy::ForOverwrite);
    imageBounds.forEach([this, imageData](const math::IVec3<int>& index)
      {
        m_Data(index) = imageData[m_Data.bounds().linearIndexOf(index)];
      });

    // Free original data
    stbi_image_free(imageData);
  }

  int Image::width() const { return m_Data.bounds().max.j + 1; }
  int Image::height() const { return m_Data.bounds().max.i + 1; }
  int Image::channels() const { return m_Data.bounds().max.k + 1; }
  int Image::pixelCount() const { return width() * height(); }
  const uint8_t* Image::data() const { return m_Data.data(); }

  math::Float4 Image::averageColor() const
  {
    ENG_CORE_ASSERT(channels() <= 4, "Image has more than four channels!");

    math::Float4 totalColor(0);
    m_Data.bounds().forEach([this, &totalColor](const math::IVec3<int>& index)
      {
        totalColor[index.k] += m_Data(index) / 255.0f;
      });

    math::Float4 averageColor = totalColor / pixelCount();
    if (channels() < 4)
      averageColor.a = 1.0f;

    return averageColor;
  }



  Texture::~Texture() = default;

  std::unique_ptr<Texture> Texture::Create(uint32_t width, uint32_t height)
  {
    switch (RendererAPI::GetAPI())
    {
      case RendererAPI::API::OpenGL:        return std::make_unique<OpenGLTexture>(width, height);
      case RendererAPI::API::OpenGL_Legacy: return std::make_unique<OpenGLTexture>(width, height);
      default:                              throw  std::invalid_argument("Unknown RendererAPI!");
    }
  }

  std::unique_ptr<Texture> Texture::Create(const std::filesystem::path& path)
  {
    switch (RendererAPI::GetAPI())
    {
      case RendererAPI::API::OpenGL:        return std::make_unique<OpenGLTexture>(path);
      case RendererAPI::API::OpenGL_Legacy: return std::make_unique<OpenGLTexture>(path);
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