#include "ENpch.h"
#include "OpenGLTexture.h"
#include "Engine/Threads/Threads.h"
#include "Engine/Debug/Instrumentor.h"

#include <glad/glad.h>

static constexpr uint32_t c_MipmapLevels = 8;
static constexpr float c_AnistropicFilteringAmount = 16.0f;

namespace eng
{
  static GLenum internalFormatOf(const Image& image)
  {
    switch (image.channels())
    {
      case 3:   return GL_RGB8;
      case 4:   return GL_RGBA8;
      default:  throw std::invalid_argument("Format not supported!");
    }
  }

  static GLenum dataFormatOf(const Image& image)
  {
    switch (image.channels())
    {
      case 3:   return GL_RGB;
      case 4:   return GL_RGBA;
      default:  throw std::invalid_argument("Format not supported!");
    }
  }



  OpenGLTexture::OpenGLTexture(uint32_t width, uint32_t height)
    : m_Width(width),
      m_Height(height),
      m_RendererID(0),
      m_InternalFormat(GL_RGBA8),
      m_DataFormat(GL_RGBA)
  {
    EN_PROFILE_FUNCTION();
    EN_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");

    glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
    glTextureStorage2D(m_RendererID, 1, m_InternalFormat, m_Width, m_Height);

    glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);
  }

  OpenGLTexture::OpenGLTexture(const std::filesystem::path& path)
    : m_Width(0),
      m_Height(0),
      m_RendererID(0)
  {
    EN_PROFILE_FUNCTION();
    EN_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");

    Image image(path);
    m_Width = image.width();
    m_Height = image.height();
    m_InternalFormat = internalFormatOf(image);
    m_DataFormat = dataFormatOf(image);

    glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
    glTextureStorage2D(m_RendererID, 1, m_InternalFormat, m_Width, m_Height);

    glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat, GL_UNSIGNED_BYTE, image.data());
  }

  OpenGLTexture::~OpenGLTexture()
  {
    EN_PROFILE_FUNCTION();
    EN_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");

    glDeleteTextures(1, &m_RendererID);
  }

  uint32_t OpenGLTexture::getWidth() const { return m_Width; }
  uint32_t OpenGLTexture::getHeight() const { return m_Height; }
  uint32_t OpenGLTexture::getRendererID() const { return m_RendererID; }

  void OpenGLTexture::setData(void* data, uint32_t size)
  {
    EN_PROFILE_FUNCTION();
    EN_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");

    EN_CORE_ASSERT(size == m_Width * m_Height * (m_DataFormat == GL_RGBA ? 4 : 3), "Data must be entire texture!");
    glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat, GL_UNSIGNED_BYTE, data);
  }

  void OpenGLTexture::bind(uint32_t slot) const
  {
    EN_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");
    glBindTextureUnit(slot, m_RendererID);
  }

  bool OpenGLTexture::operator==(const Texture& other) const
  {
    return m_RendererID == other.getRendererID();
  }



  OpenGLTextureArray::OpenGLTextureArray(uint32_t textureCount, uint32_t textureSize)
    : m_MaxTextures(textureCount),
      m_TextureSize(textureSize),
      m_TextureCount(0),
      m_RendererID(0),
      m_InternalFormat(GL_RGBA8),
      m_DataFormat(GL_RGBA)
  {
    EN_PROFILE_FUNCTION();
    EN_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");

    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &m_RendererID);
    glTextureStorage3D(m_RendererID, c_MipmapLevels, m_InternalFormat, m_TextureSize, m_TextureSize, m_MaxTextures);
  }

  OpenGLTextureArray::~OpenGLTextureArray()
  {
    EN_PROFILE_FUNCTION();
    EN_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");

    glDeleteTextures(1, &m_RendererID);
  }

  void OpenGLTextureArray::bind(uint32_t slot) const
  {
    EN_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");
    glBindTextureUnit(slot, m_RendererID);
  }

  void OpenGLTextureArray::addTexture(const std::filesystem::path& path)
  {
    EN_CORE_ASSERT(path.string().size() > 0, "Filepath is an empty string!");
    addTexture(Image(path));
  }

  void OpenGLTextureArray::addTexture(const Image& image)
  {
    EN_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");
    EN_CORE_ASSERT(image.width() == m_TextureSize && image.height() == m_TextureSize, "Texture has incorrect size!");
    EN_CORE_ASSERT(internalFormatOf(image) == m_InternalFormat && dataFormatOf(image) == m_DataFormat, "Texture has incorrect format!");
    EN_CORE_ASSERT(m_TextureCount < m_MaxTextures, "Textures added has exceeded maximum textures!");

    glTextureSubImage3D(m_RendererID, 0, 0, 0, m_TextureCount, m_TextureSize,
      m_TextureSize, 1, m_DataFormat, GL_UNSIGNED_BYTE, image.data());

    glGenerateTextureMipmap(m_RendererID);

    glTextureParameterf(m_RendererID, GL_TEXTURE_MAX_ANISOTROPY, c_AnistropicFilteringAmount);

    glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    m_TextureCount++;
  }
}