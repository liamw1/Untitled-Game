#include "ENpch.h"
#include "OpenGLTextureAPI.h"
#include "Engine/Threading/Threads.h"
#include "Engine/Debug/Instrumentor.h"

static constexpr uint32_t s_MipmapLevels = 8;
static constexpr float s_AnistropicFilteringAmount = 16.0f;

namespace Engine
{
  void OpenGLTextureAPI::create2D(uint32_t binding, uint32_t width, uint32_t height)
  {
    EN_PROFILE_FUNCTION();
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::GetMainThreadID(), "OpenGL calls must be made in main thread!");
    EN_CORE_ASSERT(binding < s_MaxTextureBindings, "Binding exceeds maximum allowed texture bindings!");

    if (m_RendererIDs[binding] != 0)
      EN_CORE_WARN("Texture has already been allocated at binding {0}!", binding);

    TextureSpecs specs{};
    specs.width = width;
    specs.height = height;
    specs.internalFormat = GL_RGBA8;
    specs.dataFormat = GL_RGBA;
    specs.type = GL_TEXTURE_2D;
    specs.count = 1;

    uint32_t rendererID;
    glCreateTextures(specs.type, 1, &rendererID);
    glTextureStorage2D(rendererID, 1, specs.internalFormat, specs.width, specs.height);

    glTextureParameteri(rendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(rendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(rendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(rendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);

    m_RendererIDs[binding] = rendererID;
    m_TextureSpecifications[binding] = specs;

    // TODO: Check if sum of allocations exceed texture size limits
  }

  void OpenGLTextureAPI::create2D(uint32_t binding, const std::string& path)
  {
    EN_PROFILE_FUNCTION();
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::GetMainThreadID(), "OpenGL calls must be made in main thread!");
    EN_CORE_ASSERT(binding < s_MaxTextureBindings, "Binding exceeds maximum allowed texture bindings!");
    EN_CORE_ASSERT(path.size() > 0, "Filepath is an empty string!");

    if (m_RendererIDs[binding] != 0)
      EN_CORE_WARN("Texture has already been allocated at binding {0}!", binding);

    TextureSpecs specs{};
    specs.type = GL_TEXTURE_2D;
    specs.count = 1;
    stbi_uc* data = LoadTextureFromImage(specs, path);

    uint32_t rendererID;
    glCreateTextures(specs.type, 1, &rendererID);
    glTextureStorage2D(rendererID, 1, specs.internalFormat, specs.width, specs.height);

    glTextureParameteri(rendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(rendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(rendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(rendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTextureSubImage2D(rendererID, 0, 0, 0, specs.width, specs.height, specs.dataFormat, GL_UNSIGNED_BYTE, data);

    m_RendererIDs[binding] = rendererID;
    m_TextureSpecifications[binding] = specs;

    stbi_image_free(data);
  }

  void OpenGLTextureAPI::create2DArray(uint32_t binding, uint32_t textureCount, uint32_t textureSize)
  {
    EN_PROFILE_FUNCTION();
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::GetMainThreadID(), "OpenGL calls must be made in main thread!");
    EN_CORE_ASSERT(binding < s_MaxTextureBindings, "Binding exceeds maximum allowed texture bindings!");

    if (m_RendererIDs[binding] != 0)
      EN_CORE_WARN("Texture has already been allocated at binding {0}!", binding);

    TextureSpecs specs{};
    specs.width = textureSize;
    specs.height = textureSize;
    specs.internalFormat = GL_RGBA8;
    specs.dataFormat = GL_RGBA;
    specs.type = GL_TEXTURE_2D_ARRAY;
    specs.count = 0;
    specs.maxCount = textureCount;

    uint32_t rendererID;
    glCreateTextures(specs.type, 1, &rendererID);
    glTextureStorage3D(rendererID, s_MipmapLevels, specs.internalFormat, textureSize, textureSize, textureCount);

    m_RendererIDs[binding] = rendererID;
    m_TextureSpecifications[binding] = specs;
  }

  void OpenGLTextureAPI::remove(uint32_t binding)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::GetMainThreadID(), "OpenGL calls must be made in main thread!");
    EN_CORE_ASSERT(binding < s_MaxTextureBindings, "Binding exceeds maximum allowed texture bindings!");

    glDeleteTextures(1, &m_RendererIDs[binding]);
    m_RendererIDs[binding] = 0;
    m_TextureSpecifications[binding] = {};
  }

  void OpenGLTextureAPI::bind(uint32_t binding) const
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::GetMainThreadID(), "OpenGL calls must be made in main thread!");
    EN_CORE_ASSERT(binding < s_MaxTextureBindings, "Binding exceeds maximum allowed texture bindings!");
    glBindTextureUnit(binding, m_RendererIDs[binding]);
  }

  void OpenGLTextureAPI::add(uint32_t binding, const std::string& path)
  {
    EN_PROFILE_FUNCTION();
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::GetMainThreadID(), "OpenGL calls must be made in main thread!");
    EN_CORE_ASSERT(binding < s_MaxTextureBindings, "Binding exceeds maximum allowed texture bindings!");
    EN_CORE_ASSERT(m_TextureSpecifications[binding].type == GL_TEXTURE_2D_ARRAY, "Specified binding is not a textrure array!");
    EN_CORE_ASSERT(m_TextureSpecifications[binding].count < m_TextureSpecifications[binding].maxCount, "Adding texture would exceed maximum texture count!");
    EN_CORE_ASSERT(path.size() > 0, "Filepath is an empty string!");

    TextureSpecs textureSpecs{};
    TextureSpecs& arraySpecs = m_TextureSpecifications[binding];
    stbi_uc* data = LoadTextureFromImage(textureSpecs, path);
    arraySpecs.count++;

    EN_CORE_ASSERT(textureSpecs.width == arraySpecs.width, "Loaded texture {0} has incorrect width!", path);
    EN_CORE_ASSERT(textureSpecs.height == arraySpecs.height, "Loaded texture {0} has incorrect height!", path);
    EN_CORE_ASSERT(textureSpecs.internalFormat == arraySpecs.internalFormat, "Loaded texture {0} has incorrect internal format!", path);
    EN_CORE_ASSERT(textureSpecs.dataFormat == arraySpecs.dataFormat, "Loaded texture {0} has incorrect data format!", path);

    glTextureSubImage3D(m_RendererIDs[binding], 0, 0, 0, arraySpecs.count, arraySpecs.width,
                        arraySpecs.height, 1, arraySpecs.dataFormat, GL_UNSIGNED_BYTE, data);

    glGenerateTextureMipmap(m_RendererIDs[binding]);

    glTextureParameterf(m_RendererIDs[binding], GL_TEXTURE_MAX_ANISOTROPY, s_AnistropicFilteringAmount);

    glTextureParameteri(m_RendererIDs[binding], GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(m_RendererIDs[binding], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_RendererIDs[binding], GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(m_RendererIDs[binding], GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(data);
  }

  stbi_uc* OpenGLTextureAPI::LoadTextureFromImage(TextureSpecs& specs, const std::string& path)
  {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    stbi_uc* data = nullptr;
    {
      EN_PROFILE_SCOPE("stbi_load --> OpenGLTexture2D::OpenGLTexture2D(const std::string&)");
      data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    }
    EN_CORE_ASSERT(data, "Failed to load texture at {0}!", path);
    specs.width = width;
    specs.height = height;

    if (channels == 4)
    {
      specs.internalFormat = GL_RGBA8;
      specs.dataFormat = GL_RGBA;
    }
    else if (channels == 3)
    {
      specs.internalFormat = GL_RGB8;
      specs.dataFormat = GL_RGB;
    }
    EN_CORE_ASSERT(specs.internalFormat * specs.dataFormat != 0, "Format not supported!");

    return data;
  }
}
