#pragma once
#include "Engine/Renderer/TextureAPI.h"
#include <glad/glad.h>

#include <codeanalysis\warnings.h> // Disable intellisense warnings
#pragma warning(push)
#pragma warning(disable : ALL_CODE_ANALYSIS_WARNINGS)
#include <stb_image.h>
#pragma warning(pop)

namespace Engine
{
  class OpenGLTextureAPI : public TextureAPI
  {
  public:
    void create2D(uint32_t binding, uint32_t width, uint32_t height);
    void create2D(uint32_t binding, const std::string & path);
    void create2DArray(uint32_t binding, uint32_t textureCount, uint32_t textureSize);
    void remove(uint32_t binding);

    void bind(uint32_t binding) const;

    void add(uint32_t binding, const std::string& path);

  private:
    struct TextureSpecs
    {
      uint32_t width;
      uint32_t height;
      GLenum internalFormat;
      GLenum dataFormat;

      GLenum type;
      uint32_t count;
      uint32_t maxCount = 1;
    };

    static stbi_uc* LoadTextureFromImage(TextureSpecs& specs, const std::string& path);

    static constexpr int s_MaxTextureBindings = 32; // NOTE: Should query this from OpenGL
    std::array<uint32_t, s_MaxTextureBindings> m_RendererIDs;
    std::array<TextureSpecs, s_MaxTextureBindings> m_TextureSpecifications;
  };
}