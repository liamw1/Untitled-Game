#pragma once
#include <cstdint>
#include <string>

namespace Engine
{
  class TextureAPI
  {
  public:
    virtual ~TextureAPI() = default;

    virtual void create2D(uint32_t binding, uint32_t width, uint32_t height) = 0;
    virtual void create2D(uint32_t binding, const std::string& path) = 0;
    virtual void create2DArray(uint32_t binding, uint32_t textureCount, uint32_t textureSize) = 0;
    virtual void remove(uint32_t binding) = 0;

    virtual void bind(uint32_t binding) const = 0;

    virtual void add(uint32_t binding, const std::string& path) = 0;
  };
}