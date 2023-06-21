#pragma once

namespace Engine
{
  namespace UniformBuffer
  {
    void Initialize();

    void Allocate(uint32_t binding, uint32_t size);
    void Deallocate(uint32_t binding);

    void Bind(uint32_t binding);
    void Unbind();

    int MaxSize();
    int GetSize(uint32_t binding);

    void SetData(uint32_t binding, const void* data);
    void SetData(uint32_t binding, const void* data, uint32_t size, uint32_t offset = 0);
  }
}