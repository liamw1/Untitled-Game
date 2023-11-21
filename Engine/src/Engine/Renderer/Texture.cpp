#include "ENpch.h"
#include "Texture.h"
#include "RendererAPI.h"
#include "Platform/OpenGL/OpenGLTexture.h"
#include "Engine/Debug/Instrumentor.h"

#include <codeanalysis\warnings.h> // Disable intellisense warnings
#pragma warning(push, 0)
#pragma warning(disable : ALL_CODE_ANALYSIS_WARNINGS)
#include <stb_image.h>
#pragma warning(pop)

namespace eng
{
  Image::Image(const std::filesystem::path& path)
    : m_Data({}, AllocationPolicy::Deferred)
  {
    ENG_PROFILE_FUNCTION();

    i32 width;
    i32 height;
    i32 channels;

    // Load image data
    stbi_set_flip_vertically_on_load(true);
    stbi_uc* imageData = stbi_load(path.string().c_str(), &width, &height, &channels, 0);
    ENG_CORE_ASSERT(imageData, "Failed to load image at {0}.", path);

    // Copy data into ArrayBox
    math::IBox3<i32> imageBounds(0, 0, 0, height - 1, width - 1, channels - 1);
    m_Data = math::ArrayBox<u8, i32>(imageBounds, AllocationPolicy::ForOverwrite);
    imageBounds.forEach([this, imageData](const math::IVec3<i32>& index)
      {
        m_Data(index) = imageData[m_Data.bounds().linearIndexOf(index)];
      });

    // Free original data
    stbi_image_free(imageData);
  }

  i32 Image::width() const { return m_Data.bounds().max.j + 1; }
  i32 Image::height() const { return m_Data.bounds().max.i + 1; }
  i32 Image::channels() const { return m_Data.bounds().max.k + 1; }
  i32 Image::pixelCount() const { return width() * height(); }
  const u8* Image::data() const { return m_Data.data(); }

  math::Float4 Image::averageColor() const
  {
    ENG_CORE_ASSERT(channels() <= 4, "Image has more than four channels!");

    math::Float4 totalColor(0);
    m_Data.bounds().forEach([this, &totalColor](const math::IVec3<i32>& index)
      {
        totalColor[index.k] += m_Data(index) / 255.0f;
      });

    math::Float4 averageColor = totalColor / pixelCount();
    if (channels() < 4)
      averageColor.a = 1.0f;

    return averageColor;
  }



  Texture::~Texture() = default;

  std::unique_ptr<Texture> Texture::Create(u32 width, u32 height)
  {
    switch (RendererAPI::GetAPI())
    {
      case RendererAPI::API::OpenGL:        return std::make_unique<OpenGLTexture>(width, height);
      case RendererAPI::API::OpenGL_Legacy: return std::make_unique<OpenGLTexture>(width, height);
    }
    throw std::invalid_argument("Invalid RendererAPI!");
  }

  std::unique_ptr<Texture> Texture::Create(const std::filesystem::path& path)
  {
    switch (RendererAPI::GetAPI())
    {
      case RendererAPI::API::OpenGL:        return std::make_unique<OpenGLTexture>(path);
      case RendererAPI::API::OpenGL_Legacy: return std::make_unique<OpenGLTexture>(path);
    }
    throw std::invalid_argument("Invalid RendererAPI!");
  }



  TextureArray::~TextureArray() = default;

  std::unique_ptr<TextureArray> TextureArray::Create(u32 textureCount, u32 textureSize)
  {
    switch (RendererAPI::GetAPI())
    {
      case RendererAPI::API::OpenGL:        return std::make_unique<OpenGLTextureArray>(textureCount, textureSize);
      case RendererAPI::API::OpenGL_Legacy: return std::make_unique<OpenGLTextureArray>(textureCount, textureSize);
    }
    throw std::invalid_argument("Invalid RendererAPI!");
  }
}