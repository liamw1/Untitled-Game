#pragma once
#include <cstdint>

namespace Engine
{
  class UniformBufferAPI
  {
  public:
    virtual ~UniformBufferAPI() = default;

    virtual void allocate(uint32_t binding, uint32_t size) = 0;
    virtual void deallocate(uint32_t binding) = 0;

    virtual void bind(uint32_t binding) const = 0;
    virtual void unbind() const = 0;

    virtual uint32_t getSize(uint32_t binding) const = 0;

    virtual void setData(uint32_t binding, const void* data, uint32_t size, uint32_t offset = 0) = 0;
  };
}