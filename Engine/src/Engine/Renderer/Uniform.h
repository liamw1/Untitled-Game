#pragma once

namespace Engine
{
  class Uniform
  {
  public:
    virtual ~Uniform() = default;

    virtual void bind() const = 0;
    virtual void unBind() const = 0;

    virtual void set(const void* data, uint32_t size) = 0;

    static std::unique_ptr<Uniform> Create(uint32_t binding, uint32_t size);
  };
}