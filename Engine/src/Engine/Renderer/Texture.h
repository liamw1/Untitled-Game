#pragma once

/*
  Abstract representation of a texture.
  Platform-specific implementation is determined by derived class.
*/
namespace Engine
{
  namespace Texture
  {
    void Initialize();

    void Create2D(uint32_t binding, uint32_t width, uint32_t height);
    void Create2D(uint32_t binding, const std::string& path);
    void Create2DArray(uint32_t binding, uint32_t textureCount, uint32_t textureSize);
    void Remove(uint32_t binding);

    void Bind(uint32_t binding);

    void Add(uint32_t binding, const std::string& path);
  }
}