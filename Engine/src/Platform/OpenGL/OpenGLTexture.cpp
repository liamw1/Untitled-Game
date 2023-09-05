#include "ENpch.h"
#include "OpenGLTexture.h"
#include "Engine/Threads/Threads.h"
#include "Engine/Debug/Instrumentor.h"

#include <stb_image.h>
#include <glad/glad.h>

static constexpr uint32_t c_MipmapLevels = 8;
static constexpr float c_AnistropicFilteringAmount = 16.0f;

namespace Engine
{
  OpenGLTexture2D::OpenGLTexture2D(uint32_t width, uint32_t height)
    : m_Width(width),
      m_Height(height),
      m_RendererID(0),
      m_InternalFormat(GL_RGBA8),
      m_DataFormat(GL_RGBA)
  {
    EN_PROFILE_FUNCTION();
    EN_CORE_ASSERT(Threads::IsMainThread(), "OpenGL calls must be made on the main thread!");

    glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
    glTextureStorage2D(m_RendererID, 1, m_InternalFormat, m_Width, m_Height);

    glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);
  }

  OpenGLTexture2D::OpenGLTexture2D(const std::string& path)
    : m_Width(0),
      m_Height(0),
      m_RendererID(0)
  {
    EN_PROFILE_FUNCTION();
    EN_CORE_ASSERT(Threads::IsMainThread(), "OpenGL calls must be made on the main thread!");

    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    stbi_uc* data = nullptr;
    {
      EN_PROFILE_SCOPE("stbi_load --> OpenGLTexture2D::OpenGLTexture2D(const std::string&)");
      data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    }
    EN_CORE_ASSERT(data, "Failed to load image!");
    m_Width = width;
    m_Height = height;

    GLenum internalFormat = 0, dataFormat = 0;
    if (channels == 4)
    {
      internalFormat = GL_RGBA8;
      dataFormat = GL_RGBA;
    }
    else if (channels == 3)
    {
      internalFormat = GL_RGB8;
      dataFormat = GL_RGB;
    }
    EN_CORE_ASSERT(internalFormat * dataFormat != 0, "Format not supported!");

    m_InternalFormat = internalFormat;
    m_DataFormat = dataFormat;

    glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
    glTextureStorage2D(m_RendererID, 1, internalFormat, m_Width, m_Height);

    glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, dataFormat, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
  }

  OpenGLTexture2D::~OpenGLTexture2D()
  {
    EN_PROFILE_FUNCTION();
    EN_CORE_ASSERT(Threads::IsMainThread(), "OpenGL calls must be made on the main thread!");

    glDeleteTextures(1, &m_RendererID);
  }

  uint32_t OpenGLTexture2D::getWidth() const { return m_Width; }
  uint32_t OpenGLTexture2D::getHeight() const { return m_Height; }
  uint32_t OpenGLTexture2D::getRendererID() const { return m_RendererID; }

  void OpenGLTexture2D::setData(void* data, uint32_t size)
  {
    EN_PROFILE_FUNCTION();
    EN_CORE_ASSERT(Threads::IsMainThread(), "OpenGL calls must be made on the main thread!");

    EN_CORE_ASSERT(size == m_Width * m_Height * (m_DataFormat == GL_RGBA ? 4 : 3), "Data must be entire texture!");
    glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat, GL_UNSIGNED_BYTE, data);
  }

  void OpenGLTexture2D::bind(uint32_t slot) const
  {
    EN_CORE_ASSERT(Threads::IsMainThread(), "OpenGL calls must be made on the main thread!");
    glBindTextureUnit(slot, m_RendererID);
  }

  bool OpenGLTexture2D::operator==(const Texture& other) const
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
    EN_CORE_ASSERT(Threads::IsMainThread(), "OpenGL calls must be made on the main thread!");

    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &m_RendererID);
    glTextureStorage3D(m_RendererID, c_MipmapLevels, m_InternalFormat, m_TextureSize, m_TextureSize, m_MaxTextures);
  }

  OpenGLTextureArray::~OpenGLTextureArray()
  {
    EN_PROFILE_FUNCTION();
    EN_CORE_ASSERT(Threads::IsMainThread(), "OpenGL calls must be made on the main thread!");

    glDeleteTextures(1, &m_RendererID);
  }

  void OpenGLTextureArray::bind(uint32_t slot) const
  {
    EN_CORE_ASSERT(Threads::IsMainThread(), "OpenGL calls must be made on the main thread!");
    glBindTextureUnit(slot, m_RendererID);
  }

  void OpenGLTextureArray::addTexture(const std::string& path)
  {
    EN_PROFILE_FUNCTION();
    EN_CORE_ASSERT(Threads::IsMainThread(), "OpenGL calls must be made on the main thread!");

    EN_CORE_ASSERT(path.size() > 0, "Filepath is an empty string!");

    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    stbi_uc* data = nullptr;
    {
      EN_PROFILE_SCOPE("stbi_load --> OpenGLTexture2D::OpenGLTexture2D(const std::string&)");
      data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    }
    EN_CORE_ASSERT(data, "Failed to load image for texture at {0}.", path);
    EN_CORE_ASSERT(width == m_TextureSize && height == m_TextureSize, "Texture has incorrect size!");

    GLenum internalFormat = 0, dataFormat = 0;
    if (channels == 4)
    {
      internalFormat = GL_RGBA8;
      dataFormat = GL_RGBA;
    }
    else if (channels == 3)
    {
      internalFormat = GL_RGB8;
      dataFormat = GL_RGB;
    }
    EN_CORE_ASSERT(internalFormat * dataFormat != 0, "Format not supported!");
    EN_CORE_ASSERT(internalFormat == m_InternalFormat && dataFormat == m_DataFormat, "Texture has incorrect format!");
    EN_CORE_ASSERT(m_TextureCount < m_MaxTextures, "Textures added has exceeded maximum textures!");

    glTextureSubImage3D(m_RendererID, 0, 0, 0, m_TextureCount, m_TextureSize,
      m_TextureSize, 1, m_DataFormat, GL_UNSIGNED_BYTE, data);

    glGenerateTextureMipmap(m_RendererID);

    glTextureParameterf(m_RendererID, GL_TEXTURE_MAX_ANISOTROPY, c_AnistropicFilteringAmount);

    glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    stbi_image_free(data);
    m_TextureCount++;
  }
}